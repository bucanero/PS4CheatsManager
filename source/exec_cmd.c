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
    output_hash = djb2_hash(game->title_id, output_hash);
    output_hash = djb2_hash(code->file, output_hash);

    LOG("input: \"%s%s%s%s%s\"", game->name, code->name, game->version, game->title_id, code->file);
    LOG("output: 0x%016lx", output_hash);
    return output_hash;
}

static void togglePatch(const game_entry_t* game, const code_entry_t* code)
{
	char hash_path[256];
	uint8_t settings[2] = {0x30, 0x0A}; // "0\n"

	uint64_t hash = patch_hash_calc(game, code);
	snprintf(hash_path, sizeof(hash_path), GOLDCHEATS_PATCH_PATH "settings/0x%" PRIx64 ".txt", hash);

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

		default:
			break;
	}

	return;
}
