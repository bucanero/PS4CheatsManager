#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <orbis/SaveData.h>

#include "cheats.h"
#include "menu.h"
#include "common.h"
#include "util.h"


uint64_t patch_hash_calc(const game_entry_t* game, const code_entry_t* code)
{
	uint64_t output_hash = 0x1505;

	output_hash = djb2_hash(game->name, output_hash);
	output_hash = djb2_hash(code->name, output_hash);
	output_hash = djb2_hash(game->version, output_hash);
	output_hash = djb2_hash(game->path, output_hash);
	output_hash = djb2_hash(code->file, output_hash);

#ifdef DEBUG_ENABLE_LOG
	LOG("input: \"%s%s%s%s%s\"", game->name, code->name, game->version, game->path, code->file);
	LOG("output: 0x%016lx", output_hash);
#endif

	return output_hash;
}

static void togglePatch(const game_entry_t* game, const code_entry_t* code)
{
	char hash_path[256];
	uint8_t settings[2] = {0x30, 0x0A}; // "0\n"

	uint64_t hash = patch_hash_calc(game, code);
	snprintf(hash_path, sizeof(hash_path), GOLDCHEATS_PATCH_PATH "settings/0x%016lx.txt", hash);
	LOG("Toggle patch (%d): %s", code->activated, hash_path);

	settings[0] += code->activated;
	if (write_buffer(hash_path, settings, sizeof(settings)) < 0)
	{
		LOG("Failed to write patch settings file %s", hash_path);
		return;
	}

	show_message("Patch \"%s\" %s", code->name, code->activated ? "Enabled" : "Disabled");
}

void execCodeCommand(code_entry_t* code, const char* codecmd)
{
	switch (codecmd[0])
	{
		case CMD_TOGGLE_PATCH:
			togglePatch(selected_entry, code);
			break;

		case UPDATE_INTERNET_CHEATS:
			if (http_download(GOLDCHEATS_URL, GOLDCHEATS_FILE, GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
			{
				int ret = extract_zip_gh(GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, GOLDCHEATS_DATA_PATH);
				if (ret > 0)
				{
					char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
					show_message("Successfully installed %d %s files\n%s", ret, "cheat", cheat_ver);
					free(cheat_ver);
				}
				else
				{
					show_message("No files extracted!");
				}
				unlink_secure(GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP);
			}
			else
			{
				show_message("No internet connection to " GOLDCHEATS_URL GOLDCHEATS_FILE " or server not available!");
			}
			break;
		case UPDATE_INTERNET_PATCHES:
			if (http_download(GOLDPATCH_URL, GOLDPATCH_FILE, GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
			{
				int ret = extract_zip_gh(GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, GOLDCHEATS_PATCH_PATH);
				if (ret > 0)
				{
					char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
					show_message("Successfully installed %d %s files\n%s", ret, "patch", patch_ver);
					free(patch_ver);
				}
				else
				{
					show_message("No files extracted!");
				}
				unlink_secure(GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP);
			}
			else
			{
				show_message("No internet connection to " GOLDPATCH_URL GOLDPATCH_FILE " or server not available!");
			}
			break;
		case UPDATE_LOCAL_CHEATS_USB:
			if (extract_zip_gh(USB0_PATH GOLDCHEATS_LOCAL_FILE, GOLDCHEATS_DATA_PATH))
			{
				char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
				show_message("Successfully installed offline %s data from USB\n%s", "cheat", cheat_ver);
				free(cheat_ver);
			}
			else
			{
				show_message("Cannot open file " USB0_PATH GOLDCHEATS_LOCAL_FILE);
			}
			break;
			case UPDATE_LOCAL_CHEATS_HDD:
			if (extract_zip_gh(GOLDCHEATS_PATH GOLDCHEATS_LOCAL_FILE, GOLDCHEATS_DATA_PATH))
			{
				char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
				show_message("Successfully installed offline %s data from HDD\n%s", "cheat", cheat_ver);
				free(cheat_ver);
			}
			else
			{
				show_message("Cannot open file " GOLDCHEATS_PATH GOLDCHEATS_LOCAL_FILE);
			}
			break;
		case UPDATE_LOCAL_PATCHES_USB:
			if (extract_zip_gh(USB0_PATH GOLDPATCH_FILE, GOLDCHEATS_PATCH_PATH))
			{
				char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
				show_message("Successfully installed offline %s data from USB\n%s", "patch", patch_ver);
				free(patch_ver);
			}
			else
			{
				show_message("Cannot open file " USB0_PATH GOLDPATCH_FILE);
			}
			break;
			case UPDATE_LOCAL_PATCHES_HDD:
			if (extract_zip_gh(GOLDCHEATS_PATH GOLDPATCH_FILE, GOLDCHEATS_PATCH_PATH))
			{
				char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
				show_message("Successfully installed offline %s data from HDD\n%s", "patch", patch_ver);
				free(patch_ver);
			}
			else
			{
				show_message("Cannot open file " GOLDCHEATS_PATH GOLDPATCH_FILE);
			}
			break;
		default:
			break;
	}

	return;
}
