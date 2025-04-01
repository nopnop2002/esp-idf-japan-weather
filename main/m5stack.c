/*
	クジラ週間天気API
	https://api.aoikujira.com/index.php?tenki

	This example code is in the Public Domain (or CC0 licensed, at your option.)

	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "driver/gpio.h"

#include "esp_http_client.h" 
#include "esp_tls.h" 
#include "cJSON.h"

#include "ili9340_sjis.h"
#include "fontx.h"
#include "cmd.h"


// for M5Stack
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define CONFIG_MOSI_GPIO 23
#define CONFIG_SCLK_GPIO 18
#define CONFIG_TFT_CS_GPIO 14
#define CONFIG_DC_GPIO 27
#define CONFIG_RESET_GPIO 33
#define CONFIG_BL_GPIO 32
#define DISPLAY_LENGTH 26
#define GPIO_INPUT_A GPIO_NUM_39
#define GPIO_INPUT_B GPIO_NUM_38
#define GPIO_INPUT_C GPIO_NUM_37

extern QueueHandle_t xQueueCmd;

static const char *TAG = "M5STACK";

/* 
	The PEM file was extracted from the output of this command:
	openssl s_client -showcerts -connect api.aoikujira.com:443

	The CA root cert is the last cert given in the chain of certs.

	To embed it in the app binary, the PEM file is named
	in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern char cert_pem_start[] asm("_binary_cert_pem_start");
extern char cert_pem_end[] asm("_binary_cert_pem_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
	static int output_len; // Stores number of bytes read
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
			//ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, content_length=%d", esp_http_client_get_content_length(evt->client));
			ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, output_len=%d", output_len);
			// If user_data buffer is configured, copy the response into the buffer
			if (evt->user_data) {
				memcpy(evt->user_data + output_len, evt->data, evt->data_len);
			}
			output_len += evt->data_len;
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
			output_len = 0;
			break;
		case HTTP_EVENT_DISCONNECTED:
			ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
			int mbedtls_err = 0;
			esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
			if (err != 0) {
				output_len = 0;
				ESP_LOGE(TAG, "Last esp error code: 0x%x", err);
				ESP_LOGE(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			break;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
		case HTTP_EVENT_REDIRECT:
			ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
			break;
#endif
	}
	return ESP_OK;
}


// Left Button Monitoring
void buttonA(void *pvParameters)
{
	ESP_LOGI(pcTaskGetName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = BUTTON_LEFT;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_reset_pin(GPIO_INPUT_A);
	gpio_set_direction(GPIO_INPUT_A, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_A);
		if (level == 0) {
			ESP_LOGI(pcTaskGetName(0), "Push Button");
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
	ESP_LOGI(pcTaskGetName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = BUTTON_MIDDLE;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_reset_pin(GPIO_INPUT_B);
	gpio_set_direction(GPIO_INPUT_B, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_B);
		if (level == 0) {
			ESP_LOGI(pcTaskGetName(0), "Push Button");
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
	ESP_LOGI(pcTaskGetName(0), "Start");
	CMD_t cmdBuf;
	cmdBuf.command = BUTTON_RIGHT;
	cmdBuf.taskHandle = xTaskGetCurrentTaskHandle();

	// set the GPIO as a input
	gpio_reset_pin(GPIO_INPUT_C);
	gpio_set_direction(GPIO_INPUT_C, GPIO_MODE_DEF_INPUT);

	while(1) {
		int level = gpio_get_level(GPIO_INPUT_C);
		if (level == 0) {
			ESP_LOGI(pcTaskGetName(0), "Push Button");
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
	
size_t http_client_content_length(char * url, char * cert_pem)
{
	ESP_LOGI(TAG, "http_client_content_length url=%s",url);
	size_t content_length;
	
	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = NULL,
		//.user_data = local_response_buffer, // Pass address of local buffer to get response
		.cert_pem = cert_pem,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGD(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
			esp_http_client_get_status_code(client),
			esp_http_client_get_content_length(client));
		content_length = esp_http_client_get_content_length(client);

	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
		content_length = 0;
	}
	esp_http_client_cleanup(client);
	return content_length;
}

esp_err_t http_client_content_get(char * url, char * cert_pem, char * response_buffer)
{
	ESP_LOGI(TAG, "http_client_content_get url=%s",url);

	esp_http_client_config_t config = {
		.url = url,
		.event_handler = _http_event_handler,
		.user_data = response_buffer, // Pass address of local buffer to get response
		.cert_pem = cert_pem,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET
	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
			esp_http_client_get_status_code(client),
			esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "\n%s", response_buffer);
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	return err;
}

void view(TFT_t *dev, FontxFile *fx, USER_DATA_t userData, uint8_t fontWidth, uint8_t fontHeight)
{
	// Clear Screen
	lcdFillScreen(dev, BLACK);
	lcdSetFontDirection(dev, 0);

	uint16_t ypos = fontHeight-1;
	uint16_t xpos = 0;
	lcdDrawUTF8String(dev, fx, xpos, ypos, userData.title, YELLOW);
	unsigned char utfs[] = u8"の週間天気予報";
	int title_size = strlen((char *)userData.title)/3; // 3Byte UTF --> 1Byte SJIS --> 2Byte ASCII
	ESP_LOGI(TAG, "title_size=%d", title_size);
	xpos = xpos + (title_size*2) * fontWidth;

	lcdDrawUTF8String(dev, fx, xpos, ypos, utfs, YELLOW);

	ypos = (fontHeight * 2.5) - 1;
	xpos = 0;
	for(int i=0; i<userData.array_size; i++) {
		ESP_LOGI(TAG, "[%d] date=[%s] forecast=[%s]", i, userData.daily[i].date, userData.daily[i].forecast);
		lcdDrawUTF8String(dev, fx, xpos, ypos, userData.daily[i].date, CYAN);

		uint16_t color = CYAN;
		unsigned char wk[16];
		strcpy((char *)wk, (char *)userData.daily[i].mintemp);
		size_t sz_mintemp = strlen((char *)wk);
		if (sz_mintemp > 1 && userData.daily[i].mintemp[0] == '-') {
			color = WHITE;
			strcpy((char *)wk, (char *)&userData.daily[i].mintemp[1]);
		}
		sz_mintemp = strlen((char *)wk);
		ESP_LOGI(TAG, "mintemp=[%s]-->[%s]", userData.daily[i].mintemp, wk);
		if (strlen((char *)userData.daily[i].mintemp) == 1) {
			lcdDrawUTF8String(dev, fx, xpos+(10*fontWidth), ypos, wk, color);
		} else {
			lcdDrawUTF8String(dev, fx, xpos+(9*fontWidth), ypos, wk, color);
		}

		uint8_t slash[2];
		strcpy((char *)slash, "/");
		lcdDrawString(dev, fx, xpos+(11*fontWidth), ypos, slash, CYAN);

		color = CYAN;
		strcpy((char *)wk, (char *)userData.daily[i].maxtemp);
		size_t sz_maxtemp = strlen((char *)wk);
		if (sz_maxtemp > 1 && userData.daily[i].maxtemp[0] == '-') {
			color = WHITE;
			strcpy((char *)wk, (char *)&userData.daily[i].maxtemp[1]);
		}
		sz_maxtemp = strlen((char *)wk);
		ESP_LOGI(TAG, "maxtemp=[%s]-->[%s]", userData.daily[i].maxtemp, wk);
		if (strlen((char *)userData.daily[i].maxtemp) == 1) {
			lcdDrawUTF8String(dev, fx, xpos+(13*fontWidth), ypos, wk, color);
		} else {
			lcdDrawUTF8String(dev, fx, xpos+(12*fontWidth), ypos, wk, color);
		}

		lcdDrawUTF8String(dev, fx, xpos+(15*fontWidth), ypos, userData.daily[i].forecast, CYAN);
		ypos = ypos + fontHeight;
	}
}

void JSON_Print(cJSON *element) {
	if (element->type == cJSON_Invalid) ESP_LOGI(TAG, "cJSON_Invalid");
	if (element->type == cJSON_False) ESP_LOGI(TAG, "cJSON_False");
	if (element->type == cJSON_True) ESP_LOGI(TAG, "cJSON_True");
	if (element->type == cJSON_NULL) ESP_LOGI(TAG, "cJSON_NULL");
	if (element->type == cJSON_Number) ESP_LOGI(TAG, "cJSON_Number int=%d double=%f", element->valueint, element->valuedouble);
	if (element->type == cJSON_String) ESP_LOGI(TAG, "cJSON_String string=%s", element->valuestring);
	if (element->type == cJSON_Array) ESP_LOGI(TAG, "cJSON_Array");
	if (element->type == cJSON_Object) {
		ESP_LOGI(TAG, "cJSON_Object");
		cJSON *child_element = NULL;
		cJSON_ArrayForEach(child_element, element) {
			JSON_Print(child_element);
		}
	}
	if (element->type == cJSON_Raw) ESP_LOGI(TAG, "cJSON_Raw");
}


void tft(void *pvParameters)
{
	USER_DATA_t userData;
	int location;
#if CONFIG_ESP_LOCATION_304
	strcpy((char *)userData.title, u8"釧路");
	location = 304;
#elif CONFIG_ESP_LOCATION_302
	strcpy((char *)userData.title, u8"旭川");
	location = 302;
#elif CONFIG_ESP_LOCATION_306
	strcpy((char *)userData.title, u8"札幌");
	location = 306;
#elif CONFIG_ESP_LOCATION_308
	strcpy((char *)userData.title, u8"青森");
	location = 308;
#elif CONFIG_ESP_LOCATION_309
	strcpy((char *)userData.title, u8"秋田");
	location = 309;
#elif CONFIG_ESP_LOCATION_312
	strcpy((char *)userData.title, u8"仙台");
	location = 312;
#elif CONFIG_ESP_LOCATION_323
	strcpy((char *)userData.title, u8"新潟");
	location = 323;
#elif CONFIG_ESP_LOCATION_325
	strcpy((char *)userData.title, u8"金沢");
	location = 325;
#elif CONFIG_ESP_LOCATION_319
	strcpy((char *)userData.title, u8"東京");
	location = 319;
#elif CONFIG_ESP_LOCATION_316
	strcpy((char *)userData.title, u8"宇都宮");
	location = 316;
#elif CONFIG_ESP_LOCATION_322
	strcpy((char *)userData.title, u8"長野");
	location = 322;
#elif CONFIG_ESP_LOCATION_329
	strcpy((char *)userData.title, u8"名古屋");
	location = 329;
#elif CONFIG_ESP_LOCATION_331
	strcpy((char *)userData.title, u8"大阪");
	location = 331;
#elif CONFIG_ESP_LOCATION_341
	strcpy((char *)userData.title, u8"高松");
	location = 341;
#elif CONFIG_ESP_LOCATION_337
	strcpy((char *)userData.title, u8"松江");
	location = 337;
#elif CONFIG_ESP_LOCATION_338
	strcpy((char *)userData.title, u8"広島");
	location = 338;
#elif CONFIG_ESP_LOCATION_344
	strcpy((char *)userData.title, u8"高知");
	location = 344;
#elif CONFIG_ESP_LOCATION_346
	strcpy((char *)userData.title, u8"福岡");
	location = 346;
#elif CONFIG_ESP_LOCATION_352
	strcpy((char *)userData.title, u8"鹿児島");
	location = 352;
#elif CONFIG_ESP_LOCATION_353
	strcpy((char *)userData.title, u8"那覇");
	location = 353;
#elif CONFIG_ESP_LOCATION_356
	strcpy((char *)userData.title, u8"石垣");
	location = 356;
#endif

	// Get Weather Information from here
	// https://api.aoikujira.com/index.php?tenki
	ESP_LOGI(pcTaskGetName(0), "location=%d", location);
	char url[64];
	//strcpy(url, "https://api.aoikujira.com/tenki/week.php?fmt=json&city=319");
	sprintf(url, "https://api.aoikujira.com/tenki/week.php?fmt=json&city=%d", location);
	ESP_LOGI(pcTaskGetName(0), "url=%s",url);

	// Get content length
	size_t content_length;
	for (int retry=0;retry<10;retry++) {
		content_length = http_client_content_length(url, cert_pem_start);
		ESP_LOGI(pcTaskGetName(0), "content_length=%d", content_length);
		if (content_length > 0) break;
		vTaskDelay(100);
	}

	if (content_length == 0) {
		ESP_LOGE(pcTaskGetName(0), "[%s] server does not respond", url);
		while(1) {
			vTaskDelay(100);
		}
	}

	char *response_buffer;	// Buffer to store response of http request from event handler
	response_buffer = (char *) malloc(content_length+1);
	if (response_buffer == NULL) {
		ESP_LOGE(pcTaskGetName(0), "Failed to allocate memory for output buffer");
		while(1) {
			vTaskDelay(1);
		}
	}
	bzero(response_buffer, content_length+1);

	// Get content
	while(1) {
		esp_err_t err = http_client_content_get(url, cert_pem_start, response_buffer);
		if (err == ESP_OK) break;
		vTaskDelay(100);
	}
	ESP_LOGI(TAG, "content_length=%d", content_length);
	ESP_LOGI(TAG, "\n[%s]", response_buffer);

	// JSON Deserialize
	cJSON *root = cJSON_Parse(response_buffer);
	int root_array_size = cJSON_GetArraySize(root); 
	ESP_LOGI(TAG, "root_array_size=%d", root_array_size);
	cJSON *element1 = cJSON_GetArrayItem(root, 0);
	JSON_Print(element1);
	cJSON *element2 = cJSON_GetArrayItem(root, 1);
	JSON_Print(element2);
	int element2_array_size = cJSON_GetArraySize(element2);
	ESP_LOGI(pcTaskGetName(0), "element2_array_size=%d", element2_array_size);
	userData.array_size = element2_array_size;
	for (int i=0;i<element2_array_size;i++) {
		cJSON *element3 = cJSON_GetArrayItem(element2, i);
		JSON_Print(element3);
		//char *date = cJSON_GetObjectItem(element3,"date")->valuestring;
		//ESP_LOGI(pcTaskGetName(0), "date=%s",date);
		strcpy((char *)userData.daily[i].date, cJSON_GetObjectItem(element3,"date")->valuestring);
		//char *forecast = cJSON_GetObjectItem(element3,"forecast")->valuestring;
		//ESP_LOGI(pcTaskGetName(0), "forecast=%s",forecast);
		strcpy((char *)userData.daily[i].forecast, cJSON_GetObjectItem(element3,"forecast")->valuestring);
		//char *mintemp = cJSON_GetObjectItem(element3,"mintemp")->valuestring;
		//ESP_LOGI(pcTaskGetName(0), "mintemp=%s",mintemp);
		strcpy((char *)userData.daily[i].mintemp, cJSON_GetObjectItem(element3,"mintemp")->valuestring);
		strcpy((char *)userData.daily[i].maxtemp, cJSON_GetObjectItem(element3,"maxtemp")->valuestring);
	}
	cJSON_Delete(root);
	free(response_buffer);

	for(int i=0; i<userData.array_size; i++) {
		ESP_LOGI(pcTaskGetName(0), "[%d] date=[%s] forecast=[%s] mintemp=[%s] maxtemp=[%s]"
			,i, userData.daily[i].date, userData.daily[i].forecast, userData.daily[i].mintemp, userData.daily[i].maxtemp);
	}


	// Open UTF to SJIS Table
	char table[64];
	sprintf(table, "/font/%s", "Utf8Sjis.tbl");
	ESP_LOGI(TAG, "table=%s", table);
	int ret = initSJIS(table);
	if (ret == 0) {
		ESP_LOGE(pcTaskGetName(0), "initSJIS fail");
	}

	// Set font file
	FontxFile fx[2];
#if CONFIG_ESP_FONT_GOTHIC
	// 12x24Dot Gothic
	InitFontx(fx,"/font/ILGH24XB.FNT","/font/ILGZ24XB.FNT");
	//InitFontx(fx,"/font/ILGH24XB.FNT","");
#endif
#if CONFIG_ESP_FONT_MINCYO
	// 12x24Dot Mincyo
	InitFontx(fx,"/font/ILMH24XB.FNT","/font/ILMZ24XB.FNT");
	//InitFontx(fx,"/font/ILMH24XB.FNT","");
#endif

	// Get font width & height
	uint8_t buffer[FontxGlyphBufSize];
	uint8_t fontWidth;
	uint8_t fontHeight;
	GetFontx(fx, 0, buffer, &fontWidth, &fontHeight);
	ESP_LOGI(pcTaskGetName(0), "fontWidth=%d fontHeight=%d",fontWidth,fontHeight);

	// Setup Screen
	TFT_t dev;
	spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_TFT_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO, CONFIG_BL_GPIO);
	lcdInit(&dev, 0x9341, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0);
	ESP_LOGI(pcTaskGetName(0), "Setup Screen done");

	int lines = (SCREEN_HEIGHT - fontHeight) / fontHeight;
	ESP_LOGD(pcTaskGetName(0), "SCREEN_HEIGHT=%d fontHeight=%d lines=%d", SCREEN_HEIGHT, fontHeight, lines);
	int ymax = (lines+1) * fontHeight;
	ESP_LOGD(pcTaskGetName(0), "ymax=%d",ymax);

	// Reset scroll area
	lcdSetScrollArea(&dev, 0, 0x0140, 0);

#if 0
	// Test code
	uint8_t utf8[3];
	uint16_t sjis[32];
	utf8[0] = 0xe6;
	utf8[1] = 0x97;
	utf8[2] = 0xa5;
	sjis[0] = UTF2SJIS(utf8);
	ESP_LOGI(pcTaskGetName(0), "sjis[0]=0x%x", sjis[0]);
	lcdDrawUTF8Char(&dev, fx, 0, 24, utf8, CYAN);
#endif

	view(&dev, fx, userData, fontWidth, fontHeight);

	CMD_t cmdBuf;
	while(1) {
		xQueueReceive(xQueueCmd, &cmdBuf, portMAX_DELAY);
		ESP_LOGI(pcTaskGetName(0),"cmdBuf.command=%d", cmdBuf.command);
		if (cmdBuf.command == BUTTON_LEFT) break;
	}

	// restart
	esp_restart();
}

