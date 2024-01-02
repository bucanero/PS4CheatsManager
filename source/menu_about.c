#include <unistd.h>
#include <string.h>
#include <math.h>

#include "cheats.h"
#include "menu.h"
#include "menu_gui.h"
#include "libfont.h"

#define FONT_W  32
#define FONT_H  64
#define STEP_X  -4         // horizontal displacement

static int sx = SCREEN_WIDTH;
const char *menu_about_strings[] = {
									"Bucanero", "Developer",
									"", "",
									"GoldHEN", "credits",
									"SiSTRo", "Shiningami",
									"illusion", "Kameleon",
									"", "",
									"PS3", "credits",
									"Dnawrkshp (Artemis)", "Berion (GUI design)",
									NULL };

/***********************************************************************
* Draw a string of chars, amplifing y by sin(x)
***********************************************************************/
static void draw_sinetext(int y, const char* string)
{
    int x = sx;       // every call resets the initial x
    int sl = strlen(string);
    char tmp[2] = {0, 0};
    float amp;

    SetFontSize(FONT_W, FONT_H);
    for(int i = 0; i < sl; i++)
    {
        amp = sinf(x      // testing sinf() from math.h
                 * 0.01)  // it turns out in num of bends
                 * 10;    // +/- vertical bounds over y

        if(x > 0 && x < SCREEN_WIDTH - FONT_W)
        {
            tmp[0] = string[i];
            DrawStringMono(x, y + amp, tmp);
        }

        x += FONT_W;
    }

    //* Move string by defined step
    sx += STEP_X;

    if(sx + (sl * FONT_W) < 0)           // horizontal bound, then loop
        sx = SCREEN_WIDTH + FONT_W;
}

static void _draw_LeonLuna(void)
{
	DrawTextureCenteredY(&menu_textures[leon_luna_jpg_index], 0, SCREEN_HEIGHT/2, 0, menu_textures[leon_luna_jpg_index].width, menu_textures[leon_luna_jpg_index].height, 0xFFFFFF00 | 0xFF);
	DrawTexture(&menu_textures[help_png_index], 0, 840, 0, SCREEN_WIDTH + 20, 100, 0xFFFFFF00 | 0xFF);

	SetFontColor(APP_FONT_MENU_COLOR | 0xFF, 0);
	draw_sinetext(860, "... in memory of Leon & Luna - may your days be filled with eternal joy ...");
}

static void _draw_AboutMenu(u8 alpha)
{
	//------------- About Menu Contents
	DrawTextureCenteredX(&menu_textures[titlescr_logo_png_index], SCREEN_WIDTH/2, 160, 0, menu_textures[titlescr_logo_png_index].width/2, menu_textures[titlescr_logo_png_index].height/2, 0xFFFFFF00 | alpha);

	SetFontAlign(FONT_ALIGN_SCREEN_CENTER);
	SetCurrentFont(font_console_regular);
	SetFontColor(APP_FONT_MENU_COLOR | alpha, 0);
	SetFontSize(APP_FONT_SIZE_JARS);
	DrawStringMono(0, 250, "PlayStation 4 Cheats Manager");

	for (int cnt = 0; menu_about_strings[cnt] != NULL; cnt += 2)
	{
		SetFontAlign(FONT_ALIGN_RIGHT);
		DrawStringMono((SCREEN_WIDTH / 2) - 20, 350 + (cnt * 25), menu_about_strings[cnt]);

		SetFontAlign(FONT_ALIGN_LEFT);
		DrawStringMono((SCREEN_WIDTH / 2) + 20, 350 + (cnt * 25), menu_about_strings[cnt + 1]);
	}

	SetFontAlign(FONT_ALIGN_SCREEN_CENTER);
	DrawStringMono(0, 850, "\xB0\xB1\xB2\xDB\xB3 in memory of Leon & Luna \xB3\xDB\xB2\xB1\xB0");
	SetFontAlign(FONT_ALIGN_LEFT);
}

void Draw_AboutMenu_Ani(void)
{
	for (int ani = 0; ani < MENU_ANI_MAX; ani++)
	{
		SDL_RenderClear(renderer);
		DrawBackground2D(0xFFFFFFFF);

		DrawHeader_Ani(header_ico_abt_png_index, "About", "v" CHEATSMGR_VERSION, APP_FONT_TITLE_COLOR, 0xffffffff, ani, 12);

		//------------- About Menu Contents
		u8 icon_a = (u8)(((ani * 2) > 0xFF) ? 0xFF : (ani * 2));
		int _game_a = (int)(icon_a - (MENU_ANI_MAX / 2)) * 2;
		if (_game_a > 0xFF)
			_game_a = 0xFF;
		u8 about_a = (u8)(_game_a < 0 ? 0 : _game_a);

		_draw_AboutMenu(about_a);

		SDL_RenderPresent(renderer);

		if (about_a == 0xFF)
			return;
	}
}

void Draw_AboutMenu(int ll)
{
	if (ll)
		return(_draw_LeonLuna());

	DrawHeader(header_ico_abt_png_index, 0, "About", "v" CHEATSMGR_VERSION, APP_FONT_TITLE_COLOR | 0xFF, 0xffffffff, 0);
	_draw_AboutMenu(0xFF);
}
