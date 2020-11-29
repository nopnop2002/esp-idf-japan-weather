/* Yahoo Weather

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <expat.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
#include "freertos/timers.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#include "esp_http_client.h" 
#include "esp_tls.h" 

#include "ili9340.h"
#include "fontx.h"
#include "wifi_sta.h"

static const char *TAG = "WEATHER";

#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	240
#define CS_GPIO			14
#define DC_GPIO			27
#define RESET_GPIO		33
#define BL_GPIO			32
#define DISPLAY_LENGTH	26
#define GPIO_INPUT_A	GPIO_NUM_39
#define GPIO_INPUT_B	GPIO_NUM_38
#define GPIO_INPUT_C	GPIO_NUM_37

typedef struct {
	uint16_t command;
	TaskHandle_t taskHandle;
} CMD_t;

#define CMD_VIEW1		100
#define CMD_VIEW2		200
#define CMD_UPDATE		300

typedef struct {
	unsigned char	title[64];			// "【 31日（金） 西部（名古屋） 】"	
	unsigned char	description[64];	// "晴れ"
	unsigned char	temp[64];			// "33℃/25℃"
} daily_t;

typedef struct {
	int		depth;						// XML depth
	char	tag[64];					// XML tag
	unsigned char	title[64];			// "Yahoo!天気・災害 - 西部（名古屋）の天気"
	int		titleIndex;					// index of daily.title
	int		descriptionIndex;			// index of daily.description
	daily_t	daily[8];					// See above
} user_data_t;

static QueueHandle_t xQueueCmd;

/* This project use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define ESP_WIFI_SSID "mywifissid"
*/

#define ESP_WIFI_SSID		CONFIG_ESP_WIFI_SSID
#define ESP_WIFI_PASS		CONFIG_ESP_WIFI_PASSWORD
#define ESP_MAXIMUM_RETRY	CONFIG_ESP_MAXIMUM_RETRY
#define ESP_LOCATION		CONFIG_ESP_LOCATION


/* Root cert for weather.yahoo.co.jp, taken from weather_yahoo_cert.pem

   The PEM file was extracted from the output of this command:
   openssl s_client -showcerts -connect weather.yahoo.co.jp:443 </dev/null

   The CA root cert is the last cert given in the chain of certs.

   To embed it in the app binary, the PEM file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern const char weather_yahoo_cert_pem_start[] asm("_binary_weather_yahoo_cert_pem_start");
extern const char weather_yahoo_cert_pem_end[]	asm("_binary_weather_yahoo_cert_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static char *output_buffer;  // Buffer to store response of http request from event handler
	static int output_len;		 // Stores number of bytes read
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, content_length=%d", esp_http_client_get_content_length(evt->client));
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, output_len=%d", output_len);
			// If user_data buffer is configured, copy the response into the buffer
			if (evt->user_data) {
				memcpy(evt->user_data + output_len, evt->data, evt->data_len);
			} else {
				if (output_buffer == NULL && esp_http_client_get_content_length(evt->client) > 0) {
					output_buffer = (char *) malloc(esp_http_client_get_content_length(evt->client));
					output_len = 0;
					if (output_buffer == NULL) {
						ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
						return ESP_FAIL;
					}
				}
				memcpy(output_buffer + output_len, evt->data, evt->data_len);
			}
			output_len += evt->data_len;
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			if (output_buffer != NULL) {
				// Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
				// ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
				free(output_buffer);
				output_buffer = NULL;
			}
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
			int mbedtls_err = 0;
			esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
			if (err != 0) {
				if (output_buffer != NULL) {
					free(output_buffer);
					output_buffer = NULL;
				}
				output_len = 0;
				ESP_LOGE(TAG, "Last esp error code: 0x%x", err);
				ESP_LOGE(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			break;
	}
	return ESP_OK;
}


// Left Button Monitoring
void buttonA(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = CMD_VIEW1;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT_A);
	gpio_set_direction(GPIO_INPUT_A, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_A);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			while(1) {
				level = gpio_get_level(GPIO_INPUT_A);
				if (level == 1) break;
				vTaskDelay(1);
			}
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}

// Middle Button Monitoring
void buttonB(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = CMD_VIEW2;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT_B);
	gpio_set_direction(GPIO_INPUT_B, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_B);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			while(1) {
				level = gpio_get_level(GPIO_INPUT_B);
				if (level == 1) break;
				vTaskDelay(1);
			}
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}

// Right Button Monitoring
void buttonC(void *pvParameters)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = CMD_UPDATE;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_pad_select_gpio(GPIO_INPUT_C);
	gpio_set_direction(GPIO_INPUT_C, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_C);
		if (level == 0) {
			ESP_LOGI(pcTaskGetTaskName(0), "Push Button");
			while(1) {
				level = gpio_get_level(GPIO_INPUT_C);
				if (level == 1) break;
				vTaskDelay(1);
			}
			xQueueSend(xQueueCmd, &cmdBuf, 0);
		}
		vTaskDelay(1);
	}
}

static void XMLCALL start_element(void *userData, const XML_Char *name, const XML_Char **atts)
{
	ESP_LOGD(TAG, "start_element name=%s", name);
	user_data_t *user_data = (user_data_t *) userData;
	int depth = user_data->depth;
	if (depth == 0) {
		strcpy(user_data->tag, name);
	} else {
		strcat(user_data->tag, "/");
		strcat(user_data->tag, name);
	}
	++user_data->depth;
}

static void XMLCALL end_element(void *userData, const XML_Char *name)
{
	ESP_LOGD(TAG, "end_element name[%d]=%s", strlen(name), name);
	user_data_t *user_data = (user_data_t *) userData;
	int tagLen = strlen(user_data->tag);
	int offset = tagLen - strlen(name) -1;
	user_data->tag[offset] = 0;
	ESP_LOGD(TAG, "tag=[%s]", user_data->tag);
	//int depth = user_data->depth;
	--user_data->depth;
}

static size_t getOneChar(char * src, int offset, char * dst) {
	size_t size = 4;
	if (src[offset] < 0x80) {
		size = 1;
	} else if (src[offset] < 0xE0) {
		size = 2;
	} else if (src[offset] < 0xF0) {
		size = 3;
	}
	for (int i=0; i<size; i++) {
		dst[i] = src[offset+i];
		//printf("0x%x ", dst[i]);
	}
	//printf("\n");
	return size;
}
	
static void data_handler(void *userData, const XML_Char *s, int len)
{
	user_data_t *user_data = (user_data_t *) userData;
	//int depth = user_data->depth;
	ESP_LOGD(TAG, "tag=[%s]", user_data->tag);
	ESP_LOGD(TAG, "depth=%d len=%d s=[%.*s]", user_data->depth, len, len, s);

	int offset = 0;
	char dst[4];
	int startOffset = 0;
	int endOffset = 0;
	int copyLength = 0;

	if (strcmp(user_data->tag, "rss/channel/title") == 0) {
		char start[3] = {0xe5, 0xae, 0xb3};
		while(1) {
			int res = getOneChar((char *)s, offset, dst);
			ESP_LOGD(TAG, "offset=%d len=%d res=%d", offset, len, res);
			if (strncmp(dst, start, 3) == 0) startOffset = offset + 6;
			offset = offset + res;
			if (offset == len) break;
		}
		copyLength = len - startOffset;
		strncpy((char *)user_data->title, &s[startOffset], copyLength);
		user_data->title[copyLength] = 0;
	} else if (strcmp(user_data->tag, "rss/channel/item/title") == 0) {
		int titleIndex = user_data->titleIndex;
		if (titleIndex < 8) {
			char start[3] = {0xe3, 0x80, 0x90};
			char end[3] = {0xef, 0xbc, 0x89};
			while(1) {
				int res = getOneChar((char *)s, offset, dst);
				ESP_LOGD(TAG, "offset=%d len=%d res=%d", offset, len, res);
				if (strncmp(dst, start, 3) == 0) startOffset = offset + 4;
				if (strncmp(dst, end, 3) == 0 && endOffset == 0) endOffset = offset + 3;
				offset = offset + res;
				if (offset == len) break;
			}
			copyLength = endOffset - startOffset;
			ESP_LOGD(TAG, "StartOffset=%d endOffset=%d copyLength=%d", startOffset, endOffset, copyLength);
			strncpy((char *)user_data->daily[titleIndex].title, &s[startOffset],  copyLength);
			user_data->daily[titleIndex].title[copyLength] = 0;
			user_data->titleIndex++;
		}
	} else if (strcmp(user_data->tag, "rss/channel/item/description") == 0) {
		int descriptionIndex = user_data->descriptionIndex;
		if (descriptionIndex < 8) {
			char start[1] = {0x2d};
			while(1) {
				int res = getOneChar((char *)s, offset, dst);
				ESP_LOGD(TAG, "offset=%d len=%d res=%d", offset, len, res);
				if (strncmp(dst, start, 1) == 0) startOffset = offset;
				offset = offset + res;
				if (offset == len) break;
			}
			copyLength = len - startOffset - 2;
			strncpy((char *)user_data->daily[descriptionIndex].description, s, startOffset-1);
			user_data->daily[descriptionIndex].description[startOffset-1] = 0;
			strncpy((char *)user_data->daily[descriptionIndex].temp, &s[startOffset+2], copyLength);
			user_data->daily[descriptionIndex].temp[copyLength] = 0;
			user_data->descriptionIndex++;
		}
	}
}



size_t http_client_content_length(char * url)
{
	ESP_LOGI(TAG, "http_client_content_length url=%s",url);
	size_t content_length;
	
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		//.user_data = local_response_buffer,		   // Pass address of local buffer to get response
		.cert_pem = weather_yahoo_cert_pem_start,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		content_length = esp_http_client_get_content_length(client);

	} else {
		ESP_LOGW(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		content_length = 0;
	}
	esp_http_client_cleanup(client);
	return content_length;
}

esp_err_t http_client_content_get(char * url, char * response_buffer)
{
	ESP_LOGI(TAG, "http_client_content_get url=%s",url);

	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = response_buffer,		   // Pass address of local buffer to get response
		.cert_pem = weather_yahoo_cert_pem_start,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "\n%s", response_buffer);
	} else {
		ESP_LOGW(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return err;
}



void http_client_get_user(char * url, user_data_t * userData)
{
	// Get content length from event handler
	size_t content_length;
	while (1) {
		content_length = http_client_content_length(url);
		ESP_LOGI(TAG, "content_length=%d", content_length);
		if (content_length > 0) break;
		vTaskDelay(100);
	}

	// Allocate buffer to store response of http request from event handler
	char *response_buffer;
	response_buffer = (char *) malloc(content_length+1);
	if (response_buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
		while(1) {
			vTaskDelay(1);
		}
	}
	bzero(response_buffer, content_length+1);

	// Get content from event handler
	while(1) {
		esp_err_t err = http_client_content_get(url, response_buffer);
		if (err == ESP_OK) break;
		vTaskDelay(100);
	}
	ESP_LOGI(TAG, "content_length=%d", content_length);
	ESP_LOGI(TAG, "\n[%s]", response_buffer);

	// Parse XML
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, userData);
	XML_SetElementHandler(parser, start_element, end_element);
	XML_SetCharacterDataHandler(parser, data_handler);
	if (XML_Parse(parser, response_buffer, content_length, 1) != XML_STATUS_OK) {
		ESP_LOGE(TAG, "XML_Parse fail");
		while(1) {
			vTaskDelay(1);
		}
	}
	XML_ParserFree(parser);
	free(response_buffer);
}

void view1(TFT_t *dev, FontxFile *fx, int fd, user_data_t userData, uint8_t fontWidth, uint8_t fontHeight)
{
	// Clear Screen
	lcdFillScreen(dev, BLACK);
	lcdSetFontDirection(dev, 0);

	uint16_t ypos = fontHeight-1;
	uint16_t xpos = 0;
	lcdDrawUTF8String(dev, fx, fd, xpos, ypos, userData.title, YELLOW);

	ypos = (fontHeight * 3) - 1;
	xpos = 0;
	for(int i=0; i<8; i++) {
		//ypos = (fontHeight * (3 + i)) - 1;
		//xpos = 0;
		ESP_LOGI(TAG, "daily[%d] title=[%s]", i, userData.daily[i].title);
		lcdDrawUTF8String(dev, fx, fd, xpos, ypos, userData.daily[i].title, CYAN);
		ESP_LOGI(TAG, "daily[%d] description=[%s]", i, userData.daily[i].description);
		lcdDrawUTF8String(dev, fx, fd, xpos+(10*fontWidth), ypos, userData.daily[i].description, CYAN);
		ypos = ypos + fontHeight;
	}
}

void view2(TFT_t *dev, FontxFile *fx, int fd, user_data_t userData, uint8_t fontWidth, uint8_t fontHeight)
{
	// Clear Screen
	lcdFillScreen(dev, BLACK);
	lcdSetFontDirection(dev, 0);

	uint16_t ypos = fontHeight-1;
	uint16_t xpos = 0;
	lcdDrawUTF8String(dev, fx, fd, xpos, ypos, userData.title, YELLOW);

	ypos = (fontHeight * 3) - 1;
	xpos = 0;
	for(int i=0; i<8; i++) {
		//ypos = (fontHeight * (3 + i)) - 1;
		//xpos = 0;
		ESP_LOGI(TAG, "daily[%d] title=[%s]", i, userData.daily[i].title);
		lcdDrawUTF8String(dev, fx, fd, xpos, ypos, userData.daily[i].title, CYAN);
		ESP_LOGI(TAG, "daily[%d] description=[%s]", i, userData.daily[i].temp);
		lcdDrawUTF8String(dev, fx, fd, xpos+(10*fontWidth), ypos, userData.daily[i].temp, CYAN);
		ypos = ypos + fontHeight;
	}
}

void tft(void *pvParameters)
{
	// Get Weather Information
	ESP_LOGI(pcTaskGetTaskName(0), "location=%d",ESP_LOCATION);
	char url[64];
	//https://rss-weather.yahoo.co.jp/rss/days/5110.xml
	sprintf(url, "https://rss-weather.yahoo.co.jp/rss/days/%d.xml", ESP_LOCATION);
	ESP_LOGI(pcTaskGetTaskName(0), "url=%s",url);
	user_data_t userData;
	userData.depth = 0;
	memset(userData.tag, 0, sizeof(userData.tag));
	userData.titleIndex = 0;
	userData.descriptionIndex = 0;

	// Read the content from the WEB and set it to userData
	http_client_get_user(url, &userData);

#if 0
	// Get content length
	size_t content_length;
	while (1) {
		content_length = http_client_content_length(url);
		ESP_LOGI(TAG, "content_length=%d", content_length);
		if (content_length > 0) break;
		vTaskDelay(100);
	}

	char *response_buffer;	// Buffer to store response of http request from event handler
	response_buffer = (char *) malloc(content_length+1);
	if (response_buffer == NULL) {
		ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
		while(1) {
			vTaskDelay(1);
		}
	}
	bzero(response_buffer, content_length+1);

	// Get content
	while(1) {
		esp_err_t err = http_client_content_get(url, response_buffer);
		if (err == ESP_OK) break;
		vTaskDelay(100);
	}
	ESP_LOGI(TAG, "content_length=%d", content_length);
	ESP_LOGI(TAG, "\n[%s]", response_buffer);

	// Parse XML
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, &userData);
	XML_SetElementHandler(parser, start_element, end_element);
	XML_SetCharacterDataHandler(parser, data_handler);
	if (XML_Parse(parser, response_buffer, content_length, 1) != XML_STATUS_OK) {
		ESP_LOGE(TAG, "XML_Parse fail");
		while(1) {
			vTaskDelay(1);
		}
	}
	XML_ParserFree(parser);

	free(response_buffer);
#endif

	for(int i=0; i<8; i++) {
		ESP_LOGI(TAG, "daily[%d] title=[%s]", i, userData.daily[i].title);
		ESP_LOGI(TAG, "daily[%d] description=[%s]", i, userData.daily[i].description);
		ESP_LOGI(TAG, "daily[%d] temp=[%s]", i, userData.daily[i].temp);
	}

	// Open UTF to SJIS Table
	char table[64];
	sprintf(table, "/fonts/%s", UTF8toSJIS);
	ESP_LOGI(TAG, "table=%s", table);
	int fd = open(table, O_RDONLY, 0);
	if (fd == -1){
		ESP_LOGE(TAG, "fail to open UTF8toSJIS");
	}

	// Set font file
	FontxFile fx[2];
#if CONFIG_ESP_FONT_GOTHIC
	// 12x24Dot Gothic
	InitFontx(fx,"/fonts/ILGH24XB.FNT","/fonts/ILGZ24XB.FNT");
	//InitFontx(fx,"/fonts/ILGH24XB.FNT","");
#endif
#if CONFIG_ESP_FONT_MINCYO
	// 12x24Dot Mincyo
	InitFontx(fx,"/fonts/ILMH24XB.FNT","/fonts/ILMZ24XB.FNT");
	//InitFontx(fx,"/fonts/ILMH24XB.FNT","");
#endif

	// Get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	ESP_LOGI(pcTaskGetTaskName(0), "fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

	// Setup Screen
	TFT_t dev;
	spi_master_init(&dev, CS_GPIO, DC_GPIO, RESET_GPIO, BL_GPIO);
	lcdInit(&dev, 0x9341, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	ESP_LOGI(pcTaskGetTaskName(0), "Setup Screen done");

	int lines = (SCREEN_HEIGHT - fontHeight) / fontHeight;
	ESP_LOGD(pcTaskGetTaskName(0), "SCREEN_HEIGHT=%d fontHeight=%d lines=%d", SCREEN_HEIGHT, fontHeight, lines);
	int ymax = (lines+1) * fontHeight;
	ESP_LOGD(pcTaskGetTaskName(0), "ymax=%d",ymax);

	// Reset scroll area
	lcdSetScrollArea(&dev, 0, 0x0140, 0);

#if 0
	// Test code
	uint8_t utf8[3];
	uint16_t sjis[32];
	utf8[0] = 0xe3;
	utf8[1] = 0x81;
	utf8[2] = 0x82;
	sjis[0] = UTF2SJIS(fd, utf8);
#endif

	view1(&dev, fx, fd, userData, fontWidth, fontHeight);

	CMD_t cmdBuf;

	while(1) {
		xQueueReceive(xQueueCmd, &cmdBuf, portMAX_DELAY);
		ESP_LOGI(pcTaskGetTaskName(0),"cmdBuf.command=%d", cmdBuf.command);
		if (cmdBuf.command == CMD_VIEW1) {
			view1(&dev, fx, fd, userData, fontWidth, fontHeight);
		} else if (cmdBuf.command == CMD_VIEW2) {
			view2(&dev, fx, fd, userData, fontWidth, fontHeight);
		} else if (cmdBuf.command == CMD_UPDATE) {
			userData.depth = 0;
			memset(userData.tag, 0, sizeof(userData.tag));
			userData.titleIndex = 0;
			userData.descriptionIndex = 0;
			// Read the content from the WEB and set it to userData
			http_client_get_user(url, &userData);
			view1(&dev, fx, fd, userData, fontWidth, fontHeight);
		}
	}

	// nerver reach here
	while (1) {
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}
}

static void SPIFFS_Directory(char * path) {
	DIR* dir = opendir(path);
	assert(dir != NULL);
	while (true) {
		struct dirent*pe = readdir(dir);
		if (!pe) break;
		ESP_LOGI(TAG,"d_name=%s/%s d_ino=%d d_type=%x", path, pe->d_name,pe->d_ino, pe->d_type);
	}
	closedir(dir);
}

esp_err_t SPIFFS_Mount(char * path, char * label, int max_files) {
	esp_vfs_spiffs_conf_t conf = {
		.base_path = path,
		.partition_label = label,
		.max_files = max_files,
		.format_if_mount_failed =true
	};

	// Use settings defined above toinitialize and mount SPIFFS filesystem.
	// Note: esp_vfs_spiffs_register is anall-in-one convenience function.
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret ==ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret== ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
		}
		return ret;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(conf.partition_label, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG,"Failed to get SPIFFS partition information (%s)",esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG,"Partition size: total: %d, used: %d", total, used);
	}

	if (ret == ESP_OK) {
		ESP_LOGI(TAG, "Mount %s to %s success", path, label);
		SPIFFS_Directory(path);
	}
	return ret;
}

void app_main()
{
	esp_log_level_set(TAG, ESP_LOG_INFO); 
	//esp_log_level_set(TAG, ESP_LOG_DEBUG); 

	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Initialize WiFi
	if (wifi_init_sta(ESP_WIFI_SSID, ESP_WIFI_PASS, ESP_MAXIMUM_RETRY) != ESP_OK) {
		ESP_LOGE(TAG, "Connection failed");
		while(1) { vTaskDelay(1); }
	}
	
	// Initialize SPIFFS
	ESP_LOGI(TAG, "Initializing SPIFFS");
	if (SPIFFS_Mount("/fonts", "storage", 7) != ESP_OK)
	{
		ESP_LOGE(TAG, "SPIFFS mount failed");
		while(1) { vTaskDelay(1); }
	}

	// Create Queue
	xQueueCmd = xQueueCreate( 10, sizeof(CMD_t) );
	configASSERT( xQueueCmd );

	// Create Task
	xTaskCreate(buttonA, "BUTTON1", 1024*2, NULL, 2, NULL);
	xTaskCreate(buttonB, "BUTTON2", 1024*2, NULL, 2, NULL);
	xTaskCreate(buttonC, "BUTTON3", 1024*2, NULL, 2, NULL);
	xTaskCreate(tft, "TFT", 1024*8, NULL, 5, NULL);
}
