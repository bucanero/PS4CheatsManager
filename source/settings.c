#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <orbis/libkernel.h>
#include <orbis/SaveData.h>
#include <orbis/UserService.h>
#include <cjson/cJSON.h>

#include "types.h"
#include "menu.h"
#include "cheats.h"
#include "common.h"

#define ORBIS_USER_SERVICE_USER_ID_INVALID	-1

static char * sort_opt[] = {"Disabled", "by Name", "by Title ID", NULL};

static void log_callback(int sel);
static void music_callback(int sel);
static void sort_callback(int sel);
static void ani_callback(int sel);
static void overwrite_callback(int sel);
static void clearcache_callback(int sel);
static void clearpatch_callback(int sel);
static void set_pluginsperms_callback(int sel);
static void change_url_callback(int sel);

menu_option_t menu_options[] = {
	{ .name = "Background Music", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &gcm_config.music, 
		.callback = music_callback 
	},
	{ .name = "Sort Games", 
		.options = sort_opt,
		.type = APP_OPTION_LIST,
		.value = &gcm_config.doSort, 
		.callback = sort_callback 
	},
	{ .name = "Menu Animations", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &gcm_config.doAni, 
		.callback = ani_callback 
	},
	{ .name = "Version Update Check", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &gcm_config.update, 
		.callback = update_callback 
	},
	{ .name = "Overwrite Files on Update", 
		.options = NULL,
		.type = APP_OPTION_BOOL,
		.value = &gcm_config.overwrite,
		.callback = overwrite_callback
	},
	{ .name = "Change Update Download URLs",
		.options = NULL,
		.type = APP_OPTION_CALL,
		.value = NULL,
		.callback = change_url_callback 
	},
	{ .name = "\nClear Temp Folder", 
		.options = NULL, 
		.type = APP_OPTION_CALL, 
		.value = NULL, 
		.callback = clearcache_callback 
	},
	{ .name = "Clear Patch Settings Folder", 
		.options = NULL, 
		.type = APP_OPTION_CALL, 
		.value = NULL, 
		.callback = clearpatch_callback
	},
	{ .name = "Set file permissions for Plugins",
		.options = NULL,
		.type = APP_OPTION_CALL,
		.value = NULL,
		.callback = set_pluginsperms_callback
	},
	{ .name = "Enable Debug Log",
		.options = NULL,
		.type = APP_OPTION_CALL,
		.value = NULL,
		.callback = log_callback 
	},
	{ .name = NULL }
};


static void music_callback(int sel)
{
	gcm_config.music = !sel;
}

static void sort_callback(int sel)
{
	gcm_config.doSort = sel;
}

static void ani_callback(int sel)
{
	gcm_config.doAni = !sel;
}

static void overwrite_callback(int sel)
{
	gcm_config.overwrite = !sel;
}

static void clearcache_callback(int sel)
{
	LOG("Cleaning folder '%s'...", CHEATSMGR_LOCAL_CACHE);
	clean_directory(CHEATSMGR_LOCAL_CACHE);

	show_message("Local cache folder cleaned:\n" CHEATSMGR_LOCAL_CACHE);
}

static void clearpatch_callback(int sel)
{
	LOG("Cleaning folder '" GOLDPATCH_SETTINGS_PATH "'...");
	clean_directory(GOLDPATCH_SETTINGS_PATH);
	show_message("Patch settings folder cleaned:\n" GOLDPATCH_SETTINGS_PATH);
}

static void set_pluginsperms_callback(int sel)
{
	if (set_perms_directory(GOLDCHEATS_PLUGINS_PATH, 0777) == SUCCESS)
	{
		show_message("Fixed plugins file permissions:\n" GOLDCHEATS_PLUGINS_PATH);
	}
	else
	{
		show_message("Failed to set file permissions:\n" GOLDCHEATS_PLUGINS_PATH);
	}
}

static void change_url_callback(int sel)
{
	if (osk_dialog_get_text("Enter the Cheat Download URL (1/3)", gcm_config.url_cheats, sizeof(gcm_config.url_cheats)))
	{
		if (gcm_config.url_cheats[strlen(gcm_config.url_cheats)-1] != '/')
			strcat(gcm_config.url_cheats, "/");

		show_message("Cheat Download URL changed to:\n%s", gcm_config.url_cheats);
	}

	if (osk_dialog_get_text("Enter the Patch Download URL (2/3)", gcm_config.url_patches, sizeof(gcm_config.url_patches)))
	{
		if (gcm_config.url_patches[strlen(gcm_config.url_patches)-1] != '/')
			strcat(gcm_config.url_patches, "/");

		show_message("Patch Download URL changed to:\n%s", gcm_config.url_patches);
	}

	if (osk_dialog_get_text("Enter the Plugin Download URL (3/3)", gcm_config.url_plugins, sizeof(gcm_config.url_plugins)))
		show_message("Plugin Download URL changed to:\n%s", gcm_config.url_plugins);
}

void update_callback(int sel)
{
    gcm_config.update = !sel;

    if (!gcm_config.update)
        return;

	LOG("checking latest Cheats Manager version at %s", CHEATSMGR_UPDATE_URL);

	if (!http_download(CHEATSMGR_UPDATE_URL, "", CHEATSMGR_LOCAL_CACHE "ver.check", 0))
	{
		LOG("http request to %s failed", CHEATSMGR_UPDATE_URL);
		return;
	}

	char *buffer;
	long size = 0;

	buffer = readTextFile(CHEATSMGR_LOCAL_CACHE "ver.check", &size);
	cJSON *json = cJSON_Parse(buffer);

	if (!json)
	{
		LOG("JSON parse Error: %s\n", buffer);
		free(buffer);
		return;
	}

	LOG("received %u bytes", size);

	const cJSON *ver = cJSON_GetObjectItemCaseSensitive(json, "tag_name");
	const cJSON *url = cJSON_GetObjectItemCaseSensitive(json, "assets");
	url = cJSON_GetObjectItemCaseSensitive(cJSON_GetArrayItem(url, 0), "browser_download_url");

	if (!cJSON_IsString(ver) || !cJSON_IsString(url))
	{
		LOG("no name found");
		goto end_update;
	}

	LOG("latest version is %s", ver->valuestring);

	if (strcasecmp(CHEATSMGR_VERSION, ver->valuestring + 1) == 0)
	{
		LOG("no need to update");
		goto end_update;
	}

	LOG("download URL is %s", url->valuestring);

	if (show_dialog(1, "New version available! Download update?"))
	{
		char* download_path = (dir_exists("/data/pkg/") == SUCCESS) ? "/data/pkg/cheats-manager.pkg" : "/data/cheats-manager.pkg";
		if (dir_exists("/mnt/usb0/") == SUCCESS)
		{
			download_path = "/mnt/usb0/cheats-manager.pkg";
		}

		if (http_download(url->valuestring, "", download_path, 1))
			show_message("Update downloaded to %s", download_path);
		else
			show_message("Unable to download update package file!");
	}

end_update:
	cJSON_Delete(json);
	free(buffer);
	return;
}

static void log_callback(int sel)
{
	dbglogger_init_mode(FILE_LOGGER, CHEATSMGR_PATH "cheatsmgr.log", 0);
	show_message("Debug Logging Enabled!\n\n" CHEATSMGR_PATH "cheatsmgr.log");
}

int save_app_settings(app_config_t* config)
{
	char filePath[256];
	OrbisSaveDataMount2 mount;
	OrbisSaveDataDirName dirName;
	OrbisSaveDataMountResult mountResult;

	memset(&mount, 0x00, sizeof(mount));
	memset(&mountResult, 0x00, sizeof(mountResult));
	strlcpy(dirName.data, "Settings", sizeof(dirName.data));

	mount.userId = gcm_config.user_id;
	mount.dirName = &dirName;
	mount.blocks = ORBIS_SAVE_DATA_BLOCKS_MIN2;
	mount.mountMode = (ORBIS_SAVE_DATA_MOUNT_MODE_CREATE2 | ORBIS_SAVE_DATA_MOUNT_MODE_RDWR | ORBIS_SAVE_DATA_MOUNT_MODE_COPY_ICON);

	if (sceSaveDataMount2(&mount, &mountResult) < 0) {
		LOG("sceSaveDataMount2 ERROR");
		return 0;
	}

	LOG("Saving Settings...");
	snprintf(filePath, sizeof(filePath), CHEATSMGR_SANDBOX_PATH "settings.bin", mountResult.mountPathName);
	write_buffer(filePath, (uint8_t*) config, sizeof(app_config_t));

	orbis_UpdateSaveParams(mountResult.mountPathName, "PS4 Cheats Manager", "User Settings", "www.bucanero.com.ar");
	orbis_SaveUmount(mountResult.mountPathName);

	return 1;
}

int load_app_settings(app_config_t* config)
{
	char filePath[256];
	app_config_t* file_data;
	size_t file_size;
	OrbisSaveDataMount2 mount;
	OrbisSaveDataDirName dirName;
	OrbisSaveDataMountResult mountResult;

	if (sceSaveDataInitialize3(0) != SUCCESS)
	{
		LOG("Failed to initialize save data library");
		return 0;
	}

	memset(&mount, 0x00, sizeof(mount));
	memset(&mountResult, 0x00, sizeof(mountResult));
	strlcpy(dirName.data, "Settings", sizeof(dirName.data));

	mount.userId = gcm_config.user_id;
	mount.dirName = &dirName;
	mount.blocks = ORBIS_SAVE_DATA_BLOCKS_MIN2;
	mount.mountMode = ORBIS_SAVE_DATA_MOUNT_MODE_RDONLY;

	if (sceSaveDataMount2(&mount, &mountResult) < 0) {
		LOG("sceSaveDataMount2 ERROR");
		return 0;
	}

	LOG("Loading Settings...");
	snprintf(filePath, sizeof(filePath), CHEATSMGR_SANDBOX_PATH "settings.bin", mountResult.mountPathName);

	if (read_buffer(filePath, (uint8_t**) &file_data, &file_size) == SUCCESS && file_size == sizeof(app_config_t))
	{
		file_data->user_id = config->user_id;
		memcpy(config, file_data, file_size);

		LOG("%s %s Settings loaded: UserID (%08x)", config->app_name, config->app_ver, config->user_id);
		LOG("M[%d] A[%d] S[%d] U[%d] W[%d]", config->music, config->doAni, config->doSort, config->update, config->overwrite);
		free(file_data);
	}

	orbis_SaveUmount(mountResult.mountPathName);

	return 1;
}
