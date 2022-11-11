#ifndef __ARTEMIS_MENU_H__
#define __ARTEMIS_MENU_H__

#include <SDL2/SDL.h>

#include "types.h"
#include "settings.h"

// SDL window and software renderer
extern SDL_Window* window;
extern SDL_Renderer* renderer;

//Textures
enum texture_index
{
//Artemis assets
	bgimg_png_index,
	bglist_png_index,
	cheat_png_index,
	goldhen_png_index,
	circle_error_dark_png_index,
	circle_error_light_png_index,
	circle_loading_bg_png_index,
	circle_loading_seek_png_index,
	edit_ico_add_png_index,
	edit_ico_del_png_index,
	edit_shadow_png_index,
	footer_ico_circle_png_index,
	footer_ico_cross_png_index,
	footer_ico_lt_png_index,
	footer_ico_rt_png_index,
	footer_ico_square_png_index,
	footer_ico_triangle_png_index,
	header_dot_png_index,
	header_ico_abt_png_index,
	header_ico_cht_png_index,
	header_ico_opt_png_index,
	header_ico_xmb_png_index,
	header_line_png_index,
	help_png_index,
	mark_arrow_png_index,
	mark_line_png_index,
	mark_line_nowork_png_index,
	opt_off_png_index,
	opt_on_png_index,
	scroll_bg_png_index,
	scroll_lock_png_index,
	titlescr_ico_xmb_png_index,
	titlescr_ico_cht_png_index,
	titlescr_ico_pat_png_index,
	titlescr_ico_net_png_index,
	titlescr_ico_opt_png_index,
	titlescr_ico_abt_png_index,
	titlescr_logo_png_index,

	TOTAL_MENU_TEXTURES
};

#define RGBA_R(c)		(uint8_t)((c & 0xFF000000) >> 24)
#define RGBA_G(c)		(uint8_t)((c & 0x00FF0000) >> 16)
#define RGBA_B(c)		(uint8_t)((c & 0x0000FF00) >> 8)
#define RGBA_A(c)		(uint8_t) (c & 0x000000FF)

//Fonts
#define  font_console_regular				0

#define APP_FONT_COLOR						0xFFFFFF00
#define APP_FONT_TAG_COLOR					0xFFFFFF00
#define APP_FONT_MENU_COLOR					0x00000000
#define APP_FONT_TITLE_COLOR				0x00000000
#define APP_FONT_SIZE_TITLE					84, 72
#define APP_FONT_SIZE_SUBTITLE				68, 60
#define APP_FONT_SIZE_SUBTEXT				36, 36
#define APP_FONT_SIZE_ABOUT					56, 50
#define APP_FONT_SIZE_SELECTION				56, 48
#define APP_FONT_SIZE_DESCRIPTION			54, 48
#define APP_FONT_SIZE_MENU					56, 54
#define APP_FONT_SIZE_JARS					32, 64
#define APP_LINE_OFFSET						40

#define SCREEN_WIDTH						1920
#define SCREEN_HEIGHT						1080

//Asset sizes
#define help_png_x							80
#define help_png_y							150
#define help_png_w							1730
#define help_png_h							800


typedef struct pad_input
{
    uint32_t idle;    // idle time
    uint32_t pressed; // button pressed in last frame
    uint32_t down;    // button is currently down
    uint32_t active;  // button is pressed in last frame, or held down for a long time (10 frames)
} pad_input_t;

typedef struct t_png_texture
{
	uint32_t *buffer;
	int width;
	int height;
	u32 size;
	SDL_Texture *texture;
} png_texture;

extern u32 * texture_mem;      // Pointers to texture memory
extern u32 * free_mem;         // Pointer after last texture

extern png_texture * menu_textures;				// png_texture array for main menu, initialized in LoadTexture

extern int highlight_alpha;						// Alpha of the selected
extern int idle_time;							// Set by readPad

extern const char * menu_about_strings[];

extern int menu_id;
extern int menu_sel;
extern int menu_old_sel[]; 
extern int last_menu_id[];
extern const char * menu_pad_help[];

extern struct game_entry * selected_entry;
extern struct code_entry * selected_centry;
extern int option_index;

void DrawBackground2D(u32 rgba);
void DrawTexture(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba);
void DrawTextureCentered(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba);
void DrawTextureCenteredX(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba);
void DrawTextureCenteredY(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba);
void DrawHeader(int icon, int xOff, const char * headerTitle, const char * headerSubTitle, u32 rgba, u32 bgrgba, int mode);
void DrawHeader_Ani(int icon, const char * headerTitle, const char * headerSubTitle, u32 rgba, u32 bgrgba, int ani, int div);
void DrawBackgroundTexture(int x, int id, u8 alpha);
void DrawTextureRotated(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba, float angle);
void Draw_MainMenu();
void Draw_MainMenu_Ani();
int LoadMenuTexture(const char* path, int idx);

void drawSplashLogo(int m);
void drawEndLogo();

int load_app_settings(app_config_t* config);
int save_app_settings(app_config_t* config);
int reset_app_settings(app_config_t* config);

int initialize_jbc();
void terminate_jbc();
int patch_save_libraries();
int unpatch_SceShellCore();

#endif
