/* 
	Cheats Manager PS4 main.c
*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <orbis/Sysmodule.h>
#include <orbis/AudioOut.h>
#include <orbis/CommonDialog.h>
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>

#include "cheats.h"
#include "util.h"
#include "common.h"
#include "orbisPad.h"

//Menus
#include "menu.h"
#include "menu_gui.h"

//Font
#include "libfont.h"
#include "ttf_render.h"
#include "font-16x32.h"

//Sound
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

// Audio handle
static int32_t audio = 0;


#define load_menu_texture(name, type) \
			if (!LoadMenuTexture(GOLDCHEATS_APP_PATH "images/" #name "." #type , name##_##type##_index)) return 0;

app_config_t gcm_config = {
    .app_name = "GOLDHEN",
    .app_ver = {0},
    .music = 1,
    .doSort = 1,
    .doAni = 1,
    .update = 1,
    .overwrite = 1,
    .user_id = 0,
};

int close_app = 0;
int idle_time = 0;                          // Set by readPad

png_texture * menu_textures;                // png_texture array for main menu, initialized in LoadTexture
SDL_Window* window;                         // SDL window
SDL_Renderer* renderer;                     // SDL software renderer
uint32_t* texture_mem;                      // Pointers to texture memory
uint32_t* free_mem;                         // Pointer after last texture

const char * menu_pad_help[TOTAL_MENU_IDS] = { NULL,												//Main
								"\x10 Select    \x13 Back    \x11 Refresh",							//Update
								"\x10 Select    \x13 Back    \x12 Filter    \x11 Refresh",			//HDD list
								"\x10 Select    \x13 Back    \x12 Filter    \x11 Refresh",			//Patch list
								"\x10 Select    \x13 Back    \x12 Filter    \x11 Refresh",			//Online list
								"\x10 Select    \x13 Back",											//Options
								"\x13 Back",														//About
								"\x10 Select    \x12 View Code    \x13 Back",						//Select Cheats
								"\x13 Back",														//View Cheat
								"\x10 Select    \x13 Back",											//Cheat Option
								"\x13 Back",														//View Details
								};

/*
* HDD cheats list
*/
game_list_t hdd_cheats = {
	.icon_id = header_ico_cht_png_index,
	.title = "HDD Cheats",
    .list = NULL,
    .path = GOLDCHEATS_DATA_PATH,
    .ReadList = &ReadUserList,
    .ReadCodes = &ReadCodes,
    .UpdatePath = NULL,
};

/*
* HDD patches list
*/
game_list_t hdd_patches = {
    .icon_id = header_ico_cht_png_index,
    .title = "Game Patches",
    .list = NULL,
    .path = GOLDCHEATS_PATCH_PATH "json/",
    .ReadList = &ReadPatchList,
    .ReadCodes = &ReadPatches,
    .UpdatePath = NULL,
};

/*
* Online code list
*/
game_list_t online_cheats = {
	.icon_id = header_ico_cht_png_index,
	.title = "Online Cheats",
    .list = NULL,
    .path = ONLINE_URL,
    .ReadList = &ReadOnlineList,
    .ReadCodes = &ReadOnlineSaves,
    .UpdatePath = NULL,
};

/*
* Update cheat code list
*/
game_list_t update_cheats = {
    .icon_id = header_ico_xmb_png_index,
    .title = "Update Cheats",
    .list = NULL,
    .path = "",
    .ReadList = &ReadBackupList,
    .ReadCodes = &ReadBackupCodes,
    .UpdatePath = NULL,
};


static int initPad()
{
	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD) < 0)
		return 0;

	// Initialize the Pad library
	if (orbisPadInit() < 0)
	{
		LOG("[ERROR] Failed to initialize pad library!");
		return 0;
	}

	// Get the user ID
	gcm_config.user_id = orbisPadGetConf()->userId;

	return 1;
}

// Used only in initialization. Allocates 64 mb for textures and loads the font
static int LoadTextures_Menu()
{
	texture_mem = malloc(256 * 32 * 32 * 4);
	menu_textures = (png_texture *)calloc(TOTAL_MENU_TEXTURES, sizeof(png_texture));
	
	if(!texture_mem || !menu_textures)
		return 0; // fail!
	
	ResetFont();
	free_mem = (u32 *) AddFontFromBitmapArray((u8 *) console_font_16x32, (u8 *) texture_mem, 0, 0xFF, 16, 32, 1, BIT7_FIRST_PIXEL);
	
	if (TTFLoadFont(0, "/preinst/common/font/DFHEI5-SONY.ttf", NULL, 0) != SUCCESS ||
		TTFLoadFont(1, "/system_ex/app/NPXS20113/bdjstack/lib/fonts/SCE-PS3-RD-R-LATIN.TTF", NULL, 0) != SUCCESS)
		return 0;

	free_mem = (u32*) init_ttf_table((u8*) free_mem);
	set_ttf_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WIN_SKIP_LF);
	
	//Init Main Menu textures
	load_menu_texture(bgimg, png);
	load_menu_texture(bglist, png);
	load_menu_texture(cheat, png);
	load_menu_texture(goldhen, png);
	load_menu_texture(circle_error_dark, png);
	load_menu_texture(circle_error_light, png);
	load_menu_texture(circle_loading_bg, png);
	load_menu_texture(circle_loading_seek, png);
	load_menu_texture(edit_ico_add, png);
	load_menu_texture(edit_ico_del, png);
	load_menu_texture(edit_shadow, png);
	load_menu_texture(footer_ico_circle, png);
	load_menu_texture(footer_ico_cross, png);
	load_menu_texture(footer_ico_lt, png);
	load_menu_texture(footer_ico_rt, png);
	load_menu_texture(footer_ico_square, png);
	load_menu_texture(footer_ico_triangle, png);
	load_menu_texture(header_dot, png);
	load_menu_texture(header_ico_abt, png);
	load_menu_texture(header_ico_cht, png);
	load_menu_texture(header_ico_opt, png);
	load_menu_texture(header_ico_xmb, png);
	load_menu_texture(header_line, png);
	load_menu_texture(help, png);
	load_menu_texture(mark_arrow, png);
	load_menu_texture(mark_line, png);
	load_menu_texture(mark_line_nowork, png);
	load_menu_texture(opt_off, png);
	load_menu_texture(opt_on, png);
	load_menu_texture(scroll_bg, png);
	load_menu_texture(scroll_lock, png);
	load_menu_texture(titlescr_ico_abt, png);
	load_menu_texture(titlescr_ico_cht, png);
	load_menu_texture(titlescr_ico_pat, png);
	load_menu_texture(titlescr_ico_net, png);
	load_menu_texture(titlescr_ico_opt, png);
	load_menu_texture(titlescr_ico_xmb, png);
	load_menu_texture(titlescr_logo, png);

//	menu_textures[icon_png_file_index].texture = NULL;

	u32 tBytes = free_mem - texture_mem;
	LOG("LoadTextures_Menu() :: Allocated %db (%.02fkb, %.02fmb) for textures", tBytes, tBytes / (float)1024, tBytes / (float)(1024 * 1024));
	return 1;
}

static int LoadSounds(void* data)
{
	uint8_t* play_audio = data;
	int32_t sOffs = 0;
	drmp3 wav;

	// Decode a mp3 file to play
	if (!drmp3_init_file(&wav, GOLDCHEATS_APP_PATH "audio/background_music.mp3", NULL))
	{
		LOG("[ERROR] Failed to decode audio file");
		return -1;
	}

	// Calculate the sample count and allocate a buffer for the sample data accordingly
	size_t sampleCount = drmp3_get_pcm_frame_count(&wav) * wav.channels;
	drmp3_int16 *pSampleData = (drmp3_int16 *)malloc(sampleCount * sizeof(uint16_t));

	// Decode the wav into pSampleData  wav.totalPCMFrameCount
	drmp3_read_pcm_frames_s16(&wav, drmp3_get_pcm_frame_count(&wav), pSampleData);

	// Play the song in a loop
	while (!close_app)
	{
		if (*play_audio == 0)
		{
			usleep(0x1000);
			continue;
		}

		/* Output audio */
		sceAudioOutOutput(audio, NULL);	// NULL: wait for completion

		if (sceAudioOutOutput(audio, pSampleData + sOffs) < 0)
		{
			LOG("Failed to output audio");
			return -1;
		}

		sOffs += 256 * 2;

		if (sOffs >= sampleCount)
			sOffs = 0;
	}

	free(pSampleData);
	drmp3_uninit(&wav);

	return 0;
}

static void registerSpecialChars()
{
	// Register button icons
	RegisterSpecialCharacter(CHAR_BTN_X, 0, 1.0, &menu_textures[footer_ico_cross_png_index]);
	RegisterSpecialCharacter(CHAR_BTN_S, 0, 1.0, &menu_textures[footer_ico_square_png_index]);
	RegisterSpecialCharacter(CHAR_BTN_T, 0, 1.0, &menu_textures[footer_ico_triangle_png_index]);
	RegisterSpecialCharacter(CHAR_BTN_O, 0, 1.0, &menu_textures[footer_ico_circle_png_index]);
}

static void terminate()
{
	LOG("Exiting...");

	terminate_jbc();
	sceSystemServiceLoadExec("exit", NULL);
}

static int initInternal()
{
    // load common modules
    int ret = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
    if (ret != SUCCESS) {
        LOG("load module failed: SYSTEM_SERVICE (0x%08x)\n", ret);
        return 0;
    }

    ret = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE);
    if (ret != SUCCESS) {
        LOG("load module failed: USER_SERVICE (0x%08x)\n", ret);
        return 0;
    }

    ret = sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SAVE_DATA);
    if (ret != SUCCESS) {
        LOG("load module failed: SAVE_DATA (0x%08x)\n", ret);
        return 0;
    }

    return 1;
}


/*
	Program start
*/
s32 main(s32 argc, const char* argv[])
{
#ifdef DEBUG_ENABLE_LOG
	// Frame tracking info for debugging
	uint32_t lastFrameTicks  = 0;
	uint32_t startFrameTicks = 0;
	uint32_t deltaFrameTicks = 0;

	dbglogger_init();
#endif

	// Initialize SDL functions
	LOG("Initializing SDL");

	if (SDL_Init(SDL_INIT_VIDEO) != SUCCESS)
	{
		LOG("Failed to initialize SDL: %s", SDL_GetError());
		return (-1);
	}

	initInternal();
	http_init();
	initPad();

	// Initialize audio output library
	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_AUDIOOUT) < 0 ||
		sceAudioOutInit() != SUCCESS)
	{
		LOG("[ERROR] Failed to initialize audio output");
		return (-1);
	}

	// Open a handle to audio output device
	audio = sceAudioOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_AUDIO_OUT_PORT_TYPE_MAIN, 0, 256, 48000, ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);

	if (audio <= 0)
	{
		LOG("[ERROR] Failed to open audio on main port");
		return audio;
	}

	// Create a window context
	LOG( "Creating a window");
	window = SDL_CreateWindow("main", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	if (!window) {
		LOG("SDL_CreateWindow: %s", SDL_GetError());
		return (-1);
	}

	// Create a renderer (OpenGL ES2)
	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!renderer) {
		LOG("SDL_CreateRenderer: %s", SDL_GetError());
		return (-1);
	}

	// Initialize jailbreak
	if (!initialize_jbc())
		terminate();

	mkdirs(GOLDCHEATS_DATA_PATH);
	mkdirs(GOLDCHEATS_LOCAL_CACHE);
	mkdirs(GOLDCHEATS_PATCH_PATH "settings/");
	
	// Load freetype
	if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL) < 0)
	{
		LOG("Failed to load freetype!");
		return (-1);
	}

	// Load MsgDialog
	if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_MESSAGE_DIALOG) < 0)
	{
		LOG("Failed to load dialog!");
		return (-1);
	}

	if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_COMMON_DIALOG) < 0 ||
		sceCommonDialogInitialize() < 0)
	{
		LOG("Failed to init CommonDialog!");
		return (-1);
	}

	// register exit callback
	atexit(terminate);
	
	// Load texture
	if (!LoadTextures_Menu())
	{
		LOG("Failed to load menu textures!");
		return (-1);
	}

	// Splash screen logo (fade-in)
	drawSplashLogo(1);

	// Load application settings
	load_app_settings(&gcm_config);

	// Unpack application data on first run
	if (strncmp(gcm_config.app_ver, GOLDCHEATS_VERSION, sizeof(gcm_config.app_ver)) != 0)
	{
		if (gcm_config.overwrite && extract_zip(GOLDCHEATS_APP_PATH "misc/appdata.zip", GOLDCHEATS_PATH))
		{
			char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
			char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
			show_message("Successfully installed local application data:\n\n- %s- %s", cheat_ver, patch_ver);
			free(cheat_ver);
			free(patch_ver);
		}

		strncpy(gcm_config.app_ver, GOLDCHEATS_VERSION, sizeof(gcm_config.app_ver));
		save_app_settings(&gcm_config);
	}

	// Setup font
	SetExtraSpace(-1);
	SetCurrentFont(0);

	registerSpecialChars();
	initMenuOptions();

	// Splash screen logo (fade-out)
	drawSplashLogo(-1);
	SDL_DestroyTexture(menu_textures[goldhen_png_index].texture);
	
	//Set options
	update_callback(!gcm_config.update);

	SDL_CreateThread(&LoadSounds, "audio_thread", &gcm_config.music);

	Draw_MainMenu_Ani();

	while (!close_app)
	{
#ifdef DEBUG_ENABLE_LOG
        startFrameTicks = SDL_GetTicks();
        deltaFrameTicks = startFrameTicks - lastFrameTicks;
        lastFrameTicks  = startFrameTicks;
#endif
		// Clear the canvas
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
		SDL_RenderClear(renderer);
		orbisPadUpdate();
		drawScene();

		//Draw help
		if (menu_pad_help[menu_id])
		{
			u8 alpha = 0xFF;
			if (orbisPadGetConf()->idle > 0x100)
			{
				int dec = (orbisPadGetConf()->idle - 0x100) * 2;
				if (dec > alpha)
					dec = alpha;
				alpha -= dec;
			}

			SetFontSize(APP_FONT_SIZE_DESCRIPTION);
			SetCurrentFont(0);
			SetFontAlign(FONT_ALIGN_SCREEN_CENTER);
			SetFontColor(APP_FONT_MENU_COLOR | alpha, 0);
			DrawString(0, SCREEN_HEIGHT - 94, (char *)menu_pad_help[menu_id]);
			SetFontAlign(FONT_ALIGN_LEFT);
		}

#ifdef DEBUG_ENABLE_LOG
		// Calculate FPS and ms/frame
		SetFontColor(APP_FONT_COLOR | 0xFF, 0);
		DrawFormatString(50, 960, "FPS: %d", (1000 / deltaFrameTicks));
#endif
		// Propagate the updated window to the screen
		SDL_RenderPresent(renderer);
	}

	if (gcm_config.doAni)
		drawEndLogo();

    // Cleanup resources
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    // Stop all SDL sub-systems
    SDL_Quit();
	http_end();
	orbisPadFinish();
	return 0;
}
