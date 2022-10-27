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

uint8_t owner_sel = 0;

menu_option_t menu_options[] = {
	{ .name = "Background Music", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &gcm_config.music, 
		.callback = music_callback 
	},
	{ .name = "Sort Games", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
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
	{ .name = "\nClear Temp Folder", 
		.options = NULL, 
		.type = APP_OPTION_CALL, 
		.value = NULL, 
		.callback = clearcache_callback 
	},
	{ .name = "Enable Debug Log",
		.options = NULL,
		.type = APP_OPTION_CALL,
		.value = NULL,
		.callback = log_callback 
	},
	{ .name = NULL }
};


void music_callback(int sel)
{
	gcm_config.music = !sel;
}

void sort_callback(int sel)
{
	gcm_config.doSort = !sel;
}

void ani_callback(int sel)
{
	gcm_config.doAni = !sel;
}

void clearcache_callback(int sel)
{
	LOG("Cleaning folder '%s'...", GOLDCHEATS_LOCAL_CACHE);
	clean_directory(GOLDCHEATS_LOCAL_CACHE);

	show_message("Local cache folder cleaned:\n" GOLDCHEATS_LOCAL_CACHE);
}

void unzip_app_data(const char* zip_file)
{
	if (extract_zip(zip_file, GOLDCHEATS_DATA_PATH))
		show_message("Successfully installed local application data");

	unlink_secure(zip_file);
}

void update_callback(int sel)
{
    gcm_config.update = !sel;

    if (!gcm_config.update)
        return;

	LOG("checking latest GoldCheats version at %s", GOLDCHEATS_UPDATE_URL);

	if (!http_download(GOLDCHEATS_UPDATE_URL, "", GOLDCHEATS_LOCAL_CACHE "ver.check", 0))
	{
		LOG("http request to %s failed", GOLDCHEATS_UPDATE_URL);
		return;
	}

	char *buffer;
	long size = 0;

	buffer = readTextFile(GOLDCHEATS_LOCAL_CACHE "ver.check", &size);
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

	if (strcasecmp(GOLDCHEATS_VERSION, ver->valuestring + 1) == 0)
	{
		LOG("no need to update");
		goto end_update;
	}

	LOG("download URL is %s", url->valuestring);

	if (show_dialog(1, "New version available! Download update?"))
	{
		if (http_download(url->valuestring, "", "/data/goldcheats.pkg", 1))
			show_message("Update downloaded to /data/goldcheats.pkg");
		else
			show_message("Download error!");
	}

end_update:
	cJSON_Delete(json);
	free(buffer);
	return;
}

void log_callback(int sel)
{
	dbglogger_init_mode(FILE_LOGGER, GOLDCHEATS_PATH "goldcheats.log", 0);
	show_message("Debug Logging Enabled!\n\n" GOLDCHEATS_PATH "goldcheats.log");
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
	snprintf(filePath, sizeof(filePath), GOLDCHEATS_SANDBOX_PATH "settings.bin", mountResult.mountPathName);
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

	sceUserServiceGetNpAccountId(config->user_id, &config->account_id);
	sceKernelGetOpenPsIdForSystem(config->psid);
	config->psid[0] = ES64(config->psid[0]);
	config->psid[1] = ES64(config->psid[1]);

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
	snprintf(filePath, sizeof(filePath), GOLDCHEATS_SANDBOX_PATH "settings.bin", mountResult.mountPathName);

	if (read_buffer(filePath, (uint8_t**) &file_data, &file_size) == SUCCESS && file_size == sizeof(app_config_t))
	{
		file_data->user_id = config->user_id;
		file_data->account_id = config->account_id;
		file_data->psid[0] = config->psid[0];
		file_data->psid[1] = config->psid[1];
		memcpy(config, file_data, file_size);

		LOG("Settings loaded: UserID (%08x) AccountID (%016lX)", config->user_id, config->account_id);
		free(file_data);
	}

	orbis_SaveUmount(mountResult.mountPathName);

	return 1;
}
