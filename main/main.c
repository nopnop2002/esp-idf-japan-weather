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
static RingbufHandle_t xRingbuffer;

/* This project use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID		CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS		CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY	CONFIG_ESP_MAXIMUM_RETRY
#define EXAMPLE_ESP_LOCATION		CONFIG_ESP_LOCATION


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
	switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
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
			if (!esp_http_client_is_chunked_response(evt->client)) {
				//char buffer[512];
				ESP_LOGD(TAG, "evt->data_len=%d", evt->data_len);
				char *buffer = malloc(evt->data_len + 1);
				esp_http_client_read(evt->client, buffer, evt->data_len);
				buffer[evt->data_len] = 0;
				//ESP_LOGI(TAG, "buffer=%s", buffer);
				UBaseType_t res = xRingbufferSend(xRingbuffer, buffer, evt->data_len, pdMS_TO_TICKS(1000));
				if (res != pdTRUE) {
					ESP_LOGE(TAG, "Failed to xRingbufferSend");
				}
				free(buffer);
			}
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
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


void http_client_get(char * url, user_data_t * userData)
{
#if 0
	esp_http_client_config_t config = {
		.url = "https://rss-weather.yahoo.co.jp/rss/days/5110.xml",
		.event_handler = _http_event_handler,
		.cert_pem = weather_yahoo_cert_pem_start,
	};
#else
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.cert_pem = weather_yahoo_cert_pem_start,
	};
#endif
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));

		// Chcek http response
		if (esp_http_client_get_status_code(client) != 200) {
			ESP_LOGE(TAG, "HTTP get fail. URL is [%s]", url);
			while (1) { vTaskDelay(1); }
		}

		// Receive an item from no-split ring buffer
		int bufferSize = esp_http_client_get_content_length(client);
		char *buffer = malloc(bufferSize + 1); 
		if (buffer == NULL) {
			ESP_LOGE(TAG, "malloc fail");
			while (1) { vTaskDelay(1); }
		}
		size_t item_size;
		int	index = 0;
		while (1) {
			char *item = (char *)xRingbufferReceive(xRingbuffer, &item_size, pdMS_TO_TICKS(1000));
			if (item != NULL) {
				ESP_LOGD(TAG, "index=%d item_size=%d", index, item_size);
				for (int i = 0; i < item_size; i++) {
					//printf("%c", item[i]);
					if (index == bufferSize) {
						ESP_LOGE(TAG, "buffer overflow. index=%d bufferSize=%d", index, bufferSize);
						while (1) { vTaskDelay(1); }
					}
					buffer[index] = item[i];
					index++;
					buffer[index] = 0;
				}
				//printf("\n");
				//Return Item
				vRingbufferReturnItem(xRingbuffer, (void *)item);
			} else {
				//Failed to receive item
				ESP_LOGD(TAG, "End of receive item");
				break;
			}
		}
		ESP_LOGI(TAG, "buffer=\n%s", buffer);

		// Parse XML
        XML_Parser parser = XML_ParserCreate(NULL);
        XML_SetUserData(parser, userData);
        XML_SetElementHandler(parser, start_element, end_element);
        XML_SetCharacterDataHandler(parser, data_handler);
        if (XML_Parse(parser, buffer, bufferSize, 1) != XML_STATUS_OK) {
            ESP_LOGE(TAG, "XML_Parse fail");
        }
		XML_ParserFree(parser);

		free(buffer);

	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		while(1) {
			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
	}
	esp_http_client_cleanup(client);
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
	ESP_LOGI(pcTaskGetTaskName(0), "location=%d",EXAMPLE_ESP_LOCATION);
	char url[64];
	//https://rss-weather.yahoo.co.jp/rss/days/5110.xml
	sprintf(url, "https://rss-weather.yahoo.co.jp/rss/days/%d.xml", EXAMPLE_ESP_LOCATION);
	ESP_LOGI(pcTaskGetTaskName(0), "url=%s",url);
	user_data_t userData;
	userData.depth = 0;
	memset(userData.tag, 0, sizeof(userData.tag));
	userData.titleIndex = 0;
	userData.descriptionIndex = 0;
	http_client_get(url, &userData);
	ESP_LOGI(TAG, "title=[%s]", userData.title);
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
			http_client_get(url, &userData);
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
	if (wifi_init_sta(EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_MAXIMUM_RETRY) != ESP_OK) {
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

	// Create No Split Ring Buffer 
	xRingbuffer = xRingbufferCreate(1024*10, RINGBUF_TYPE_NOSPLIT);
	configASSERT( xRingbuffer );

	// Create Queue
	xQueueCmd = xQueueCreate( 10, sizeof(CMD_t) );
	configASSERT( xQueueCmd );

	// Create Task
	xTaskCreate(buttonA, "BUTTON1", 1024*2, NULL, 2, NULL);
	xTaskCreate(buttonB, "BUTTON2", 1024*2, NULL, 2, NULL);
	xTaskCreate(buttonC, "BUTTON3", 1024*2, NULL, 2, NULL);
	xTaskCreate(tft, "TFT", 1024*8, NULL, 5, NULL);
}
