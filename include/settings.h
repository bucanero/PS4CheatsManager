#define GOLDCHEATS_VERSION          "1.1.0"     //GoldCheats PS4 version (about menu)

#define MENU_TITLE_OFF			45			//Offset of menu title text from menu mini icon
#define MENU_ICON_OFF 			105         //X Offset to start printing menu mini icon
#define MENU_ANI_MAX 			0x80        //Max animation number
#define MENU_SPLIT_OFF			400			//Offset from left of sub/split menu to start drawing
#define OPTION_ITEM_OFF         1595        //Offset from left of settings item/value

enum app_option_type
{
    APP_OPTION_NONE,
    APP_OPTION_BOOL,
    APP_OPTION_LIST,
    APP_OPTION_INC,
    APP_OPTION_CALL,
};

typedef struct
{
	char * name;
	char * * options;
	int type;
	uint8_t * value;
	void(*callback)(int);
} menu_option_t;

typedef struct
{
    char app_name[8];
    char app_ver[8];
    uint8_t music;
    uint8_t doSort;
    uint8_t doAni;
    uint8_t update;
    uint8_t overwrite;
    uint32_t user_id;
} app_config_t;

extern menu_option_t menu_options[];

extern app_config_t gcm_config;

void log_callback(int sel);
void owner_callback(int sel);
void music_callback(int sel);
void sort_callback(int sel);
void ani_callback(int sel);
void update_callback(int sel);
void overwrite_callback(int sel);
void clearcache_callback(int sel);
void clearpatch_callback(int sel);
void upd_appdata_callback(int sel);
void unzip_app_data(const char* zip_file);
