#define CMD_VIEW1       100
#define CMD_VIEW2       200
#define CMD_UPDATE      300

typedef struct {
    uint16_t command;
    TaskHandle_t taskHandle;
} CMD_t;

typedef struct {
    unsigned char   title[64];          // "【 31日（金） 西部（名古屋） 】"
    unsigned char   description[64];    // "晴れ"
    unsigned char   temp[64];           // "33 /25 "
} DAILY_t;

typedef struct {
    int     depth;                      // XML depth
    char    tag[64];                    // XML tag
    unsigned char   title[64];          // "Yahoo!天気・災害 - 西部（名古屋）の天気"
    int     titleIndex;                 // index of daily.title
    int     descriptionIndex;           // index of daily.description
    DAILY_t daily[8];                   // See above
} USER_DATA_t;

