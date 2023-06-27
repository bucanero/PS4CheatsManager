#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <cjson/cJSON.h>
#include <orbis/SaveData.h>
#include <orbis/SystemService.h>

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

static void updNetCheats(void)
{
	if (!http_download(GOLDCHEATS_URL, GOLDCHEATS_FILE, GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
	{
		show_message("No internet connection to " GOLDCHEATS_URL GOLDCHEATS_FILE " or server not available!");
		return;
	}

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

static void updNetPatches(void)
{
	if (!http_download(GOLDPATCH_URL, GOLDPATCH_FILE, GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
	{
		show_message("No internet connection to " GOLDPATCH_URL GOLDPATCH_FILE " or server not available!");
		return;
	}

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

static void updNetPlugins(void)
{
	if (!http_download(GOLDPLUGINS_UPDATE_URL, "", GOLDCHEATS_LOCAL_CACHE "plugins.json", 0))
	{
		show_message("No internet connection to " GOLDPLUGINS_UPDATE_URL " or server not available!");
		return;
	}

	char *buffer;
	long size = 0;

	buffer = readTextFile(GOLDCHEATS_LOCAL_CACHE "plugins.json", &size);
	cJSON *json = cJSON_Parse(buffer);

	if (!json)
	{
		show_message("Failed to parse json data");
		LOG("JSON parse Error: %s", buffer);
		free(buffer);
		return;
	}

	LOG("received %u bytes", size);

	const cJSON *ver = cJSON_GetObjectItemCaseSensitive(json, "tag_name");
	const cJSON *url = cJSON_GetObjectItemCaseSensitive(json, "assets");
	url = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(url, 0), "browser_download_url");

	if (!cJSON_IsString(ver) || !cJSON_IsString(url))
	{
		LOG("no tag name or version found");
		goto end_update;
	}

	LOG("latest version is %s", ver->valuestring);
	LOG("download URL is %s", url->valuestring);

	if (http_download(url->valuestring, "", GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
	{
		LOG("Update version %s (%s) downloaded to %s", ver->valuestring, url->valuestring, GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP);
		int ret = extract_zip_gh(GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP, GOLDCHEATS_PATH);
		if (ret > 0 && set_perms_directory(GOLDCHEATS_PLUGINS_PATH, 0777) == SUCCESS)
		{
			show_message("Successfully installed %d plugins files\nPlugins version: %s", ret, ver->valuestring);
		}
		else
		{
			show_message("No plugins files extracted!");
		}
		unlink_secure(GOLDCHEATS_LOCAL_CACHE LOCAL_TEMP_ZIP);
	}
	else
	{
		show_message("Failed to download plugins from\n%s!", url->valuestring);
	}

end_update:
	cJSON_Delete(json);
	free(buffer);
	unlink_secure(GOLDCHEATS_LOCAL_CACHE "plugins.json");
	return;
}

static void updLocalCheats(const char* upd_path)
{
	if (!extract_zip_gh(upd_path, GOLDCHEATS_DATA_PATH))
	{
		show_message("Unable to extract zip\n%s", upd_path);
		return;
	}

	char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
	show_message("Successfully installed offline cheat data from\n%s\n%s", upd_path, cheat_ver);
	free(cheat_ver);
}

static void updLocalPatches(const char* upd_path)
{
	if (!extract_zip_gh(upd_path, GOLDCHEATS_PATCH_PATH))
	{
		show_message("Unable to extract zip\n%s", upd_path);
		return;
	}

	char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
	show_message("Successfully installed offline patch data from\n%s\n%s", upd_path, patch_ver);
	free(patch_ver);
}

static void updLocalPlugins(const char* upd_path)
{
	if (!extract_zip_gh(upd_path, GOLDCHEATS_PATH))
	{
		show_message("Unable to extract zip\n%s", upd_path);
		return;
	}
	if (set_perms_directory(GOLDCHEATS_PLUGINS_PATH, 0777) == SUCCESS)
	{
		show_message("Successfully installed offline plugin files from\n%s", upd_path);
	}
}

static struct tm get_local_time(void)
{
	int tz_offset = 0;
	int tz_dst = 0;
	int ret;

	if ((ret = sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_TIME_ZONE, &tz_offset)) < 0)
	{
		LOG("Failed to obtain ORBIS_SYSTEM_SERVICE_PARAM_ID_TIME_ZONE! Setting timezone offset to 0");
		LOG("sceSystemServiceParamGetInt: 0x%08X", ret_tz);
		tz_offset = 0;
	}

	if ((ret = sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_SUMMERTIME, &tz_dst)) < 0)
	{
		LOG("Failed to obtain ORBIS_SYSTEM_SERVICE_PARAM_ID_SUMMERTIME! Setting timezone daylight time savings to 0");
		LOG("sceSystemServiceParamGetInt: 0x%08X", ret_dst);
		tz_dst = 0;
	}

	time_t modifiedTime = time(NULL) + ((tz_offset + (tz_dst * 60)) * 60);
	return (*gmtime(&modifiedTime));
}

static void backupCheats(const char* dst_path)
{
	char zip_path[256];
	struct tm t = get_local_time();

	mkdirs(dst_path);
	// build file path
	snprintf(zip_path, sizeof(zip_path), "%s" GOLDCHEATS_BACKUP_PREFIX "_%d-%02d-%02d_%02d%02d%02d.zip", dst_path, t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	if (!zip_directory(_GOLDCHEATS_PATH, GOLDCHEATS_DATA_PATH, zip_path))
	{
		show_message("Failed to backup cheats to\n%s", zip_path);
		return;
	};

	show_message("Created cheats backup successfully:\n%s", zip_path);
}

static void backupPatches(const char* dst_path)
{
	char zip_path[256];
	struct tm t = get_local_time();

	mkdirs(dst_path);
	// build file path
	snprintf(zip_path, sizeof(zip_path), "%s" GOLDPATCH_BACKUP_PREFIX "_%d-%02d-%02d_%02d%02d%02d.zip", dst_path, t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	if (!zip_directory(_GOLDCHEATS_PATH, GOLDCHEATS_PATCH_PATH, zip_path))
	{
		show_message("Failed to backup patches to\n%s", zip_path);
		return;
	}

	show_message("Created patches backup successfully:\n%s", zip_path);
}

static void backupPlugins(const char* dst_path)
{
	char zip_path[256] = {0};
	struct tm t = get_local_time();

	mkdirs(dst_path);
	// build file path
	snprintf(zip_path, sizeof(zip_path), "%s" GOLDPLUGINS_BACKUP_PREFIX "_%d-%02d-%02d_%02d%02d%02d.zip", dst_path, t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

	if (!zip_directory(_GOLDCHEATS_PATH, GOLDCHEATS_PLUGINS_PATH, zip_path))
	{
		show_message("Failed to backup plugins to\n%s", zip_path);
		return;
	}

	show_message("Created plugins backup successfully:\n%s", zip_path);
}

void execCodeCommand(code_entry_t* code, const char* codecmd)
{
	switch (codecmd[0])
	{
		case CMD_TOGGLE_PATCH:
			togglePatch(selected_entry, code);
			return;

		case CMD_UPD_INTERNET_CHEATS:
			updNetCheats();
			break;

		case CMD_UPD_INTERNET_PATCHES:
			updNetPatches();
			break;

		case CMD_UPD_INTERNET_PLUGINS:
			updNetPlugins();
			break;

		case CMD_UPD_LOCAL_CHEATS:
			updLocalCheats(code->file);
			break;

		case CMD_UPD_LOCAL_PATCHES:
			updLocalPatches(code->file);
			break;

		case CMD_UPD_LOCAL_PLUGINS:
			updLocalPlugins(code->file);
			break;

		case CMD_BACKUP_CHEATS_HDD:
		case CMD_BACKUP_CHEATS_USB:
			backupCheats(codecmd[0] == CMD_BACKUP_CHEATS_USB ? USB0_PATH "backup/cheats/" : GOLDCHEATS_PATH "backup/cheats/");
			break;

		case CMD_BACKUP_PATCHES_HDD:
		case CMD_BACKUP_PATCHES_USB:
			backupPatches(codecmd[0] == CMD_BACKUP_PATCHES_USB ? USB0_PATH "backup/patches/" : GOLDCHEATS_PATH "backup/patches/");
			break;

		case CMD_BACKUP_PLUGINS_HDD:
		case CMD_BACKUP_PLUGINS_USB:
			backupPlugins(codecmd[0] == CMD_BACKUP_PLUGINS_USB ? USB0_PATH "backup/plugins/" : GOLDCHEATS_PATH "backup/plugins/");
			break;

		default:
			break;
	}
	code->activated = 0;
}
