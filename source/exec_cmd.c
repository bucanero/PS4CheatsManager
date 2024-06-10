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

static void toggleCheatFile(game_entry_t* game)
{
	char file_path[256];

	LOG("Toggle cheat file: %s", game->path);
	if (game->flags & CHEAT_FLAG_LOCKED)
	{
		snprintf(file_path, sizeof(file_path), "%s", game->path);
		*strrchr(file_path, '-') = 0;
	}
	else
		snprintf(file_path, sizeof(file_path), "%s-disabled", game->path);

	if (rename(game->path, file_path) < 0)
	{
		LOG("Failed to rename cheat file %s", file_path);
		return;
	}
	game->flags ^= CHEAT_FLAG_LOCKED;
	free(game->path);
	asprintf(&game->path, "%s", file_path);

	show_message("Cheat File \"%s\" %s", game->name, (game->flags & CHEAT_FLAG_LOCKED) ? "Disabled" : "Enabled");
}

static void updNetCheats(void)
{
	if (!http_download(gcm_config.url_cheats, GOLDCHEATS_FILE, CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
	{
		show_message("No internet connection to %s%s or server not available!", gcm_config.url_cheats, GOLDCHEATS_FILE);
		return;
	}

	int ret = extract_zip_gh(CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP, GOLDCHEATS_DATA_PATH);
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

	unlink_secure(CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP);
}

static void updNetPatches(void)
{
	if (!http_download(gcm_config.url_patches, GOLDPATCH_FILE, CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
	{
		show_message("No internet connection to %s%s or server not available!", gcm_config.url_patches, GOLDPATCH_FILE);
		return;
	}

	int ret = extract_zip_gh(CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP, GOLDCHEATS_PATCH_PATH);
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

	unlink_secure(CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP);
}

static void updNetPlugins(void)
{
	if (!http_download(gcm_config.url_plugins, "", CHEATSMGR_LOCAL_CACHE "plugins.json", 0))
	{
		show_message("No internet connection to %s or server not available!", gcm_config.url_plugins);
		return;
	}

	char *buffer;
	long size = 0;

	buffer = readTextFile(CHEATSMGR_LOCAL_CACHE "plugins.json", &size);
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

	if (http_download(url->valuestring, "", CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP, 1))
	{
		LOG("Update version %s (%s) downloaded to %s", ver->valuestring, url->valuestring, CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP);
		int ret = extract_zip_gh(CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP, GOLDHEN_PATH "/");
		if (ret > 0 && set_perms_directory(GOLDCHEATS_PLUGINS_PATH, 0777) == SUCCESS)
		{
			show_message("Successfully installed %d plugins files\nPlugins version: %s", ret, ver->valuestring);
		}
		else
		{
			show_message("No plugins files extracted!");
		}
		unlink_secure(CHEATSMGR_LOCAL_CACHE LOCAL_TEMP_ZIP);
	}
	else
	{
		show_message("Failed to download plugins from\n%s!", url->valuestring);
	}

end_update:
	cJSON_Delete(json);
	free(buffer);
	unlink_secure(CHEATSMGR_LOCAL_CACHE "plugins.json");
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
	if (!extract_zip_gh(upd_path, GOLDHEN_PATH "/"))
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
		LOG("sceSystemServiceParamGetInt: 0x%08X", ret);
		tz_offset = 0;
	}

	if ((ret = sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_SUMMERTIME, &tz_dst)) < 0)
	{
		LOG("Failed to obtain ORBIS_SYSTEM_SERVICE_PARAM_ID_SUMMERTIME! Setting timezone daylight time savings to 0");
		LOG("sceSystemServiceParamGetInt: 0x%08X", ret);
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
	if (!zip_directory(GOLDHEN_PATH, GOLDCHEATS_DATA_PATH, zip_path))
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
	if (!zip_directory(GOLDHEN_PATH, GOLDCHEATS_PATCH_PATH, zip_path))
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

	if (!zip_directory(GOLDHEN_PATH, GOLDCHEATS_PLUGINS_PATH, zip_path))
	{
		show_message("Failed to backup plugins to\n%s", zip_path);
		return;
	}

	show_message("Created plugins backup successfully:\n%s", zip_path);
}

static void decryptMC4(const game_entry_t* game)
{
	char *buffer, *dec_data;
	char xml_path[256];

	if (game->flags & CHEAT_FLAG_ONLINE)
		snprintf(xml_path, sizeof(xml_path), "%s%s", CHEATSMGR_LOCAL_CACHE, strrchr(game->path, '/') + 1);
	else
		snprintf(xml_path, sizeof(xml_path), "%s", game->path);

	LOG("Decrypting %s...", game->path);

	buffer = readTextFile(xml_path, NULL);
	if (!buffer)
	{
		show_message("Failed to open\n%s", game->path);
		return;
	}

	dec_data = mc4_decrypt(buffer);
	free(buffer);

	if (!dec_data)
	{
		show_message("Failed to decrypt\n%s", game->path);
		return;
	}

	snprintf(xml_path, sizeof(xml_path), "%s%s.xml", CHEATSMGR_PATH, strrchr(game->path, '/') + 1);
	if (write_buffer(xml_path, (uint8_t*) dec_data, strlen(dec_data)) == SUCCESS)
		show_message("MC4 file decrypted to:\n%s", xml_path);
	else
		show_message("Failed to write decrypted file %s", xml_path);

	free(dec_data);
}

static void removeCheats(void)
{
	if (!show_dialog(1, "Are you sure you want to remove all cheats?"))
		return;

	LOG("Removing all cheats...");
	clean_directory(GOLDCHEATS_DATA_PATH "json/");
	clean_directory(GOLDCHEATS_DATA_PATH "shn/");
	clean_directory(GOLDCHEATS_DATA_PATH "mc4/");

	show_message("All cheats removed!");
}

static void removePatches(void)
{
	if (!show_dialog(1, "Are you sure you want to remove all patches?"))
		return;

	LOG("Removing all patches...");
	clean_directory(GOLDCHEATS_PATCH_PATH "xml/");
	clean_directory(GOLDPATCH_SETTINGS_PATH);

	show_message("All patches removed!");
}

static void removePlugins(void)
{
	if (!show_dialog(1, "Are you sure you want to remove all plugins?"))
		return;

	LOG("Removing all plugins...");
	clean_directory(GOLDCHEATS_PLUGINS_PATH);

	show_message("All plugins removed!");
}

void execCodeCommand(code_entry_t* code, const char* codecmd)
{
	switch (codecmd[0])
	{
		case CMD_DECRYPT_MC4:
			decryptMC4(selected_entry);
			return;

		case CMD_REMOVE_CHEATS:
			removeCheats();
			break;

		case CMD_REMOVE_PATCHES:
			removePatches();
			break;

		case CMD_REMOVE_PLUGINS:
			removePlugins();
			break;

		case CMD_TOGGLE_CHEAT:
			toggleCheatFile(selected_entry);
			return;

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
			backupCheats(codecmd[0] == CMD_BACKUP_CHEATS_USB ? USB0_PATH "backup/cheats/" : CHEATSMGR_PATH "backup/cheats/");
			break;

		case CMD_BACKUP_PATCHES_HDD:
		case CMD_BACKUP_PATCHES_USB:
			backupPatches(codecmd[0] == CMD_BACKUP_PATCHES_USB ? USB0_PATH "backup/patches/" : CHEATSMGR_PATH "backup/patches/");
			break;

		case CMD_BACKUP_PLUGINS_HDD:
		case CMD_BACKUP_PLUGINS_USB:
			backupPlugins(codecmd[0] == CMD_BACKUP_PLUGINS_USB ? USB0_PATH "backup/plugins/" : CHEATSMGR_PATH "backup/plugins/");
			break;

		default:
			break;
	}
	code->activated = 0;
}
