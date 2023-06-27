#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "types.h"
#include "libfont.h"
#include "menu.h"

#include <dbglogger.h>
#define LOG dbglogger_log

#define JAR_COLUMNS 7


int LoadMenuTexture(const char* path, int idx)
{
	int d;

	LOG("Loading '%s'", path);
	if (menu_textures[idx].texture)
		SDL_DestroyTexture(menu_textures[idx].texture);

	menu_textures[idx].size = 0;
	menu_textures[idx].texture = NULL;
	menu_textures[idx].buffer = (uint32_t*) stbi_load(path, &menu_textures[idx].width, &menu_textures[idx].height, &d, STBI_rgb_alpha);

	if (!menu_textures[idx].buffer)
	{
		LOG("Error Loading texture (%s)!", path);
		return 0;
	}

	// GLES2 supports RGBA8888 only
	for (d = 0; d < menu_textures[idx].width * menu_textures[idx].height; d++)
		menu_textures[idx].buffer[d] = ES32(menu_textures[idx].buffer[d]);

	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(menu_textures[idx].buffer, menu_textures[idx].width, menu_textures[idx].height, 32, 4 * menu_textures[idx].width,
												0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);

	menu_textures[idx].texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_FreeSurface(surface);
	stbi_image_free(menu_textures[idx].buffer);

	menu_textures[idx].size = menu_textures[idx].width * menu_textures[idx].height * 4;
	menu_textures[idx].buffer = NULL;
	return 1;
}

// draw one background color in virtual 2D coordinates
void DrawBackground2D(u32 rgba)
{
	SDL_SetRenderDrawColor(renderer, RGBA_R(rgba), RGBA_G(rgba), RGBA_B(rgba), RGBA_A(rgba));

	SDL_RenderClear(renderer);
}

void _drawListBackground(int off, int icon)
{
	switch (icon)
	{
		case header_ico_cht_png_index:
		case header_ico_opt_png_index:
		case header_ico_xmb_png_index:
			DrawTexture(&menu_textures[help_png_index], help_png_x, help_png_y, 0, help_png_w, help_png_h, 0xFFFFFF00 | 0xFF);
			break;
/*
		case 999:
			DrawTexture(&menu_textures[help_png_index], help_png_x, help_png_y, 0, help_png_w, help_png_h, 0xFFFFFF00 | 0xFF);
			if (menu_textures[icon_png_file_index].size)
			{
				DrawTexture(&menu_textures[help_png_index], SCREEN_WIDTH - 404, help_png_y + 4, 0, menu_textures[icon_png_file_index].width + 8, menu_textures[icon_png_file_index].height + 8, 0xFFFFFF00 | 0xFF);
				DrawTexture(&menu_textures[icon_png_file_index], SCREEN_WIDTH - 400, help_png_y + 8, 0, menu_textures[icon_png_file_index].width, menu_textures[icon_png_file_index].height, 0xFFFFFF00 | 0xFF);
			}
			break;

		case 888:
			DrawTexture(&menu_textures[help_png_index], off + MENU_ICON_OFF, help_png_y, 0, (SCREEN_WIDTH - 75) - off - MENU_ICON_OFF, help_png_h, 0xFFFFFF00 | 0xFF);
			break;
*/
		case header_ico_abt_png_index:
			break;

		default:
			break;
	}
}

void DrawHeader_Ani(int icon, const char * hdrTitle, const char * headerSubTitle, u32 rgba, u32 bgrgba, int ani, int div)
{
	u8 icon_a = (u8)(((ani * 2) > 0xFF) ? 0xFF : (ani * 2));
	char headerTitle[44];
	snprintf(headerTitle, sizeof(headerTitle), "%.40s%s", hdrTitle, (strlen(hdrTitle) > 40 ? "..." : ""));

	//------------ Backgrounds
	
	//Background
	DrawBackgroundTexture(0, icon, (u8)bgrgba);

	_drawListBackground(0, icon);
	//------------- Menu Bar
	SetFontColor(0x000000 | icon_a, 0);
	SetFontSize(APP_FONT_SIZE_SUBTITLE);
	DrawStringMono(MENU_ICON_OFF + 10, 80, "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");

	//header mini icon
	DrawTextureCenteredX(&menu_textures[icon], MENU_ICON_OFF - 20, 32, 0, 96, 96, 0xffffff00 | icon_a);

	//header title string
	SetFontColor(rgba | icon_a, 0);
	SetFontSize(APP_FONT_SIZE_TITLE);
	DrawString(MENU_ICON_OFF + 40, 31, headerTitle);

	//header sub title string
	if (headerSubTitle)
	{
		int width = (SCREEN_WIDTH - 75) - (MENU_ICON_OFF + MENU_TITLE_OFF + WidthFromStr(headerTitle)) - 30;
		SetFontSize(APP_FONT_SIZE_SUBTITLE);
		char * tName = strdup(headerSubTitle);
		while (WidthFromStr(tName) > width)
		{
			tName[strlen(tName) - 1] = 0;
		}
		SetFontAlign(FONT_ALIGN_RIGHT);
		DrawString(SCREEN_WIDTH - 75, 35, tName);
		free(tName);
		SetFontAlign(FONT_ALIGN_LEFT);
	}
}

void DrawHeader(int icon, int xOff, const char * hdrTitle, const char * headerSubTitle, u32 rgba, u32 bgrgba, int mode)
{
	char headerTitle[44];
	snprintf(headerTitle, sizeof(headerTitle), "%.40s%s", hdrTitle, (strlen(hdrTitle) > 40 ? "..." : ""));

	//Background
	DrawBackgroundTexture(xOff, icon, (u8)bgrgba);

	_drawListBackground(xOff, icon);
	//------------ Menu Bar
	SetFontColor(0x000000FF, 0);
	SetFontSize(APP_FONT_SIZE_SUBTITLE);
	DrawStringMono(xOff + MENU_ICON_OFF + 10, 80, "\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");

	//header mini icon
	//header title string
	SetFontColor(rgba, 0);
	if (mode)
	{
		DrawTextureCenteredX(&menu_textures[icon], xOff + MENU_ICON_OFF - 12, 40, 0, 64, 64, 0xffffffff);
		SetFontSize(APP_FONT_SIZE_SUBTITLE);
		DrawString(xOff + MENU_ICON_OFF + 40, 35, headerTitle);
	}
	else
	{
		DrawTextureCenteredX(&menu_textures[icon], xOff + MENU_ICON_OFF - 20, 32, 0, 96, 96, 0xffffffff);
		SetFontSize(APP_FONT_SIZE_TITLE);
		DrawString(xOff + MENU_ICON_OFF + 40, 31, headerTitle);
	}

	//header sub title string
	if (headerSubTitle)
	{
		int width = (SCREEN_WIDTH - 75) - (MENU_ICON_OFF + MENU_TITLE_OFF + WidthFromStr(headerTitle)) - 30;
		SetFontSize(APP_FONT_SIZE_SUBTITLE);
		char * tName = strdup(headerSubTitle);
		while (WidthFromStr(tName) > width)
		{
			tName[strlen(tName) - 1] = 0;
		}
		SetFontAlign(FONT_ALIGN_RIGHT);
		DrawString(SCREEN_WIDTH - 75, 35, tName);
		free(tName);
		SetFontAlign(FONT_ALIGN_LEFT);
	}
}

void DrawBackgroundTexture(int x, int id, u8 alpha)
{
	DrawTexture(&menu_textures[(id && id != header_ico_abt_png_index) ? bglist_png_index : bgimg_png_index], x, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFFFFFF00 | alpha);
}

void DrawTexture(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba)
{
	SDL_Rect dest = {
		.x = x,
		.y = y,
		.w = w,
		.h = h,
	};

	SDL_SetTextureAlphaMod(tex->texture, RGBA_A(rgba));
	SDL_RenderCopy(renderer, tex->texture, NULL, &dest);
}

void DrawTextureCentered(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba)
{
	x -= w / 2;
	y -= h / 2;

	DrawTexture(tex, x, y, z, w, h, rgba);
}

void DrawTextureCenteredX(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba)
{
	x -= w / 2;

	DrawTexture(tex, x, y, z, w, h, rgba);
}

void DrawTextureCenteredY(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba)
{
	y -= h / 2;

	DrawTexture(tex, x, y, z, w, h, rgba);
}

void DrawTextureRotated(png_texture* tex, int x, int y, int z, int w, int h, u32 rgba, float angle)
{
	SDL_Rect dest = {
		.x = x - (w / 2),
		.y = y - (h / 2),
		.w = w,
		.h = h,
	};

	SDL_RenderCopyEx(renderer, tex->texture, NULL, &dest, angle, NULL, SDL_FLIP_NONE);
}

static void drawJar(uint8_t idx, int pos_x, int pos_y, const char* text, uint8_t alpha)
{
	uint8_t active = (menu_sel + titlescr_ico_xmb_png_index == idx);

	DrawTexture(&menu_textures[idx], pos_x, pos_y, 0, menu_textures[idx].width, menu_textures[idx].height, 0xffffff00 | (alpha == 0xFF ? (active ? 0xFF : 0x20) : alpha));

	SetFontColor(APP_FONT_MENU_COLOR | (alpha == 0xFF ? (active ? 0xFF : 0x20) : alpha), 0);
	DrawStringMono(pos_x + (menu_textures[idx].width / 2), menu_textures[idx].height + pos_y + 50, text);
}

static void drawJars(uint8_t alpha)
{
	SetFontAlign(FONT_ALIGN_CENTER);
	SetFontSize(APP_FONT_SIZE_JARS);
	SetCurrentFont(font_console_regular);

	SetFontColor(APP_FONT_MENU_COLOR | alpha, 0);
	DrawStringMono(SCREEN_WIDTH/2, 450, "PlayStation 4 Cheats Manager");

	//User Backup
	drawJar(titlescr_ico_xmb_png_index, 400, 600, (alpha == 0xFF ? "Update" : ""), alpha);

	//HDD cheats
	drawJar(titlescr_ico_cht_png_index, 600, 600, (alpha == 0xFF ? "Cheats" : ""), alpha);

	//HDD patches
	drawJar(titlescr_ico_pat_png_index, 800, 600, (alpha == 0xFF ? "Patches" : ""), alpha);

	//Online cheats
	drawJar(titlescr_ico_net_png_index, 1000, 600, (alpha == 0xFF ? "Online DB" : ""), alpha);

	//Options
	drawJar(titlescr_ico_opt_png_index, 1200, 600, (alpha == 0xFF ? "Settings" : ""), alpha);

	//About
	drawJar(titlescr_ico_abt_png_index, 1400, 600, (alpha == 0xFF ? "About" : ""), alpha);

	SetFontAlign(FONT_ALIGN_LEFT);
}

void drawSplashLogo(int mode)
{
	int ani, max;

	if (mode > 0)
	{
		ani = 0;
		max = MENU_ANI_MAX;
	}
	else
	{
		ani = MENU_ANI_MAX;
		max = 0;
	}

	for (; ani != max; ani += mode)
	{
		// clear the current display buffer
		SDL_RenderClear(renderer);
		DrawBackground2D(0x000000FF);
		
		//------------ Backgrounds
		int logo_a_t = ((ani < 0x20) ? 0 : ((ani - 0x20)*3));
		if (logo_a_t > 0xFF)
			logo_a_t = 0xFF;
		u8 logo_a = (u8)logo_a_t;

		SDL_SetTextureAlphaMod(menu_textures[goldhen_png_index].texture, logo_a);

		//App description
		DrawTextureCentered(&menu_textures[goldhen_png_index], SCREEN_WIDTH/2, SCREEN_HEIGHT /2, 0, menu_textures[goldhen_png_index].width, menu_textures[goldhen_png_index].height, 0xFFFFFF00 | logo_a);

		//flush and flip
		SDL_RenderPresent(renderer);
	}
}

void drawEndLogo(void)
{
	SDL_Rect rect = {
		.x = 0,
		.w = SCREEN_WIDTH,
	};

	for (rect.h = 0; rect.h <= SCREEN_HEIGHT/2; rect.h += 3)
	{
		// clear the current display buffer
		SDL_RenderClear(renderer);
		DrawBackground2D(0xFFFFFFFF);

		//App description
		DrawTextureCentered(&menu_textures[titlescr_logo_png_index], SCREEN_WIDTH/2, SCREEN_HEIGHT /2, 0, menu_textures[titlescr_logo_png_index].width *3/4, menu_textures[titlescr_logo_png_index].height *3/4, 0xFFFFFF00 | 0xFF);

		rect.y = 0;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
		SDL_RenderFillRect(renderer, &rect);

		rect.y = SCREEN_HEIGHT - rect.h;
		SDL_RenderFillRect(renderer, &rect);

		//flush and flip
		SDL_RenderPresent(renderer);
	}
}

static void _draw_MainMenu(uint8_t alpha)
{
	//------------ Backgrounds

	//Background
	DrawBackgroundTexture(0, 0, 0xFF);
	
	//App logo
	DrawTextureCenteredX(&menu_textures[titlescr_logo_png_index], SCREEN_WIDTH/2, 250, 0, menu_textures[titlescr_logo_png_index].width *3/2, menu_textures[titlescr_logo_png_index].height *3/2, 0xFFFFFFFF);
	
	//App description
//	DrawTextureCenteredX(&menu_textures[logo_text_png_index], SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 40, 0, menu_textures[logo_text_png_index].width * 3/2, menu_textures[logo_text_png_index].height * 3/2, 0xFFFFFF00 | 0xFF);

	//------------ Icons
	drawJars(alpha);
}

void Draw_MainMenu_Ani(void)
{
	int max = MENU_ANI_MAX, ani = 0;
	for (ani = 0; ani < max; ani++)
	{
		SDL_RenderClear(renderer);
		DrawBackground2D(0xFFFFFFFF);
		
		//------------ Backgrounds
		u8 bg_a = (u8)(ani * 2);
		if (bg_a < 0x20)
			bg_a = 0x20;
		int logo_a_t = ((ani < 0x30) ? 0 : ((ani - 0x20)*3));
		if (logo_a_t > 0xFF)
			logo_a_t = 0xFF;
		u8 logo_a = (u8)logo_a_t;
		
		//Background
		DrawBackgroundTexture(0, 0, bg_a);
		
		//App logo
		DrawTextureCenteredX(&menu_textures[titlescr_logo_png_index], SCREEN_WIDTH/2, 250, 0, menu_textures[titlescr_logo_png_index].width *3/2, menu_textures[titlescr_logo_png_index].height *3/2, 0xFFFFFF00 | logo_a);
		
		//App description
//		DrawTextureCenteredX(&menu_textures[logo_text_png_index], SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 40, 0, menu_textures[logo_text_png_index].width * 3/2, menu_textures[logo_text_png_index].height * 3/2, 0xFFFFFF00 | logo_a);

		SDL_RenderPresent(renderer);
	}
	
	max = MENU_ANI_MAX / 2;
	int rate = (0x100 / max);
	for (ani = 0; ani < max; ani++)
	{
		SDL_RenderClear(renderer);
		DrawBackground2D(0xFFFFFFFF);
		
		u8 icon_a = (u8)(((ani * rate) > 0xFF) ? 0xFF : (ani * rate));
		
		_draw_MainMenu(icon_a);
		
		SDL_RenderPresent(renderer);

		if (icon_a == 32)
			break;
	}
}

void Draw_MainMenu(void)
{
	_draw_MainMenu(0xFF);
}
