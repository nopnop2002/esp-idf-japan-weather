#define CMD_LEFT   100
#define CMD_MIDDLE 200
#define CMD_RIGHT  300

typedef struct {
    uint16_t command;
    TaskHandle_t taskHandle;
} CMD_t;

typedef struct {
    unsigned char date[32]; // "date": "27日(日)"
	unsigned char forecast[32]; // "forecast": "晴時々曇"
	unsigned char mintemp[16]; // "mintemp": "10"
	unsigned char maxtemp[16]; // "maxtemp": "18"
} DAILY_t;


typedef struct {
	int	array_size;
	unsigned char title[64];
	DAILY_t daily[8];
} USER_DATA_t;

