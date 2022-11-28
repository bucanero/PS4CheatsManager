#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <orbis/SaveData.h>
#include <cjson/cJSON.h>
#include <mxml.h>

#include "cheats.h"
#include "common.h"
#include "settings.h"
#include "util.h"
#include "sqlite3.h"

#define UTF8_CHAR_GROUP		"\xe2\x97\x86"
#define UTF8_CHAR_ITEM		"\xe2\x94\x97"
#define UTF8_CHAR_STAR		"\xE2\x98\x85"

#define CHAR_ICON_ZIP		"\x0C"
#define CHAR_ICON_COPY		"\x0B"
#define CHAR_ICON_SIGN		"\x06"
#define CHAR_ICON_USER		"\x07"
#define CHAR_ICON_LOCK		"\x08"
#define CHAR_ICON_WARN		"\x0F"


const void* sqlite3_get_sqlite3Apis();
int sqlite3_memvfs_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi);

static sqlite3* open_sqlite_db(const char* db_path)
{
	uint8_t* db_buf;
	size_t db_size;
	sqlite3 *db;
	const sqlite3_api_routines* api = sqlite3_get_sqlite3Apis();

	// Open an in-memory database to use as a handle for loading the memvfs extension
	if (sqlite3_open(":memory:", &db) != SQLITE_OK)
	{
		LOG("open :memory: %s", sqlite3_errmsg(db));
		return NULL;
	}

	sqlite3_enable_load_extension(db, 1);
	if (sqlite3_memvfs_init(db, NULL, api) != SQLITE_OK_LOAD_PERMANENTLY)
	{
		LOG("Error loading extension: %s", "memvfs");
		return NULL;
	}

	// Done with this database
	sqlite3_close(db);

	if (read_buffer(db_path, &db_buf, &db_size) != SUCCESS)
	{
		LOG("Cannot open database file: %s", db_path);
		return NULL;
	}

	// And open that memory with memvfs now that it holds a valid database
	char *memuri = sqlite3_mprintf("file:memdb?ptr=0x%p&sz=%lld&freeonclose=1", db_buf, db_size);
	LOG("Opening '%s'...", memuri);

	if (sqlite3_open_v2(memuri, &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, "memvfs") != SQLITE_OK)
	{
		LOG("Error open memvfs: %s", sqlite3_errmsg(db));
		return NULL;
	}
	sqlite3_free(memuri);

	return db;
}

void check_game_appdb(list_t* list)
{
	sqlite3* db;
	sqlite3_stmt *res;
	list_node_t* node;
	game_entry_t* item;

	if ((db = open_sqlite_db("/system_data/priv/mms/app.db")) == NULL)
		return;

	for (node = list_head(list); (item = list_get(node)); node = list_next(node))
	{
		char* query = sqlite3_mprintf("SELECT A.titleId, A.val, B.val FROM tbl_appinfo AS A INNER JOIN tbl_appinfo AS B"
			" WHERE A.key = 'APP_VER' AND B.key = 'VERSION' AND A.titleId = %Q AND B.titleId = %Q"
			" AND (((A.val >= B.val) AND A.val = %Q) OR ((A.val < B.val) AND B.val = %Q))",
			item->title_id, item->title_id, item->version, item->version);

		if (sqlite3_prepare_v2(db, query, -1, &res, NULL) == SQLITE_OK && sqlite3_step(res) == SQLITE_ROW)
		{
			LOG("Found game: %s v%s", item->title_id, item->version);
			item->flags |= CHEAT_FLAG_OWNER;
		}
		
		sqlite3_free(query);
	}

	sqlite3_close(db);
	return;
}

int orbis_SaveUmount(const char* mountPath)
{
	OrbisSaveDataMountPoint umount;

	memset(&umount, 0, sizeof(OrbisSaveDataMountPoint));
	strncpy(umount.data, mountPath, sizeof(umount.data));

	int32_t umountErrorCode = sceSaveDataUmount(&umount);
	
	if (umountErrorCode < 0)
		LOG("UMOUNT_ERROR (%X)", umountErrorCode);

	return (umountErrorCode == SUCCESS);
}

int orbis_UpdateSaveParams(const char* mountPath, const char* title, const char* subtitle, const char* details)
{
	OrbisSaveDataParam saveParams;
	OrbisSaveDataMountPoint mount;

	memset(&saveParams, 0, sizeof(OrbisSaveDataParam));
	memset(&mount, 0, sizeof(OrbisSaveDataMountPoint));

	strncpy(mount.data, mountPath, sizeof(mount.data));
	strncpy(saveParams.title, title, ORBIS_SAVE_DATA_TITLE_MAXSIZE);
	strncpy(saveParams.subtitle, subtitle, ORBIS_SAVE_DATA_SUBTITLE_MAXSIZE);
	strncpy(saveParams.details, details, ORBIS_SAVE_DATA_DETAIL_MAXSIZE);
	saveParams.mtime = time(NULL);

	int32_t setParamResult = sceSaveDataSetParam(&mount, ORBIS_SAVE_DATA_PARAM_TYPE_ALL, &saveParams, sizeof(OrbisSaveDataParam));
	if (setParamResult < 0) {
		LOG("sceSaveDataSetParam error (%X)", setParamResult);
		return 0;
	}

	return (setParamResult == SUCCESS);
}

/*
 * Function:		endsWith()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Checks to see if a ends with b
 * Arguments:
 *	a:				String
 *	b:				Potential end
 * Return:			pointer if true, NULL if false
 */
static char* endsWith(const char * a, const char * b)
{
	int al = strlen(a), bl = strlen(b);
    
	if (al < bl)
		return NULL;

	a += (al - bl);
	while (*a)
		if (toupper(*a++) != toupper(*b++)) return NULL;

	return (char*) (a - bl);
}

/*
 * Function:		readFile()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		reads the contents of a file into a new buffer
 * Arguments:
 *	path:			Path to file
 * Return:			Pointer to the newly allocated buffer
 */
char * readTextFile(const char * path, long* size)
{
	FILE *f = fopen(path, "rb");

	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (fsize <= 0)
	{
		fclose(f);
		return NULL;
	}

	char * string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);

	string[fsize] = 0;
	if (size)
		*size = fsize;

	return string;
}
/*
static code_entry_t* _createCmdCode(uint8_t type, const char* name, char code)
{
	code_entry_t* entry = (code_entry_t *)calloc(1, sizeof(code_entry_t));
	entry->type = type;
	entry->name = strdup(name);
	asprintf(&entry->codes, "%c", code);

	return entry;
}

static option_entry_t* _initOptions(int count)
{
	option_entry_t* options = (option_entry_t*)malloc(sizeof(option_entry_t));

	options->id = 0;
	options->sel = -1;
	options->size = count;
	options->line = NULL;
	options->value = malloc (sizeof(char *) * count);
	options->name = malloc (sizeof(char *) * count);

	return options;
}

static option_entry_t* _createOptions(int count, const char* name, char value)
{
	option_entry_t* options = _initOptions(count);

	asprintf(&options->name[0], "%s %d", name, 0);
	asprintf(&options->value[0], "%c%c", value, 0);
	asprintf(&options->name[1], "%s %d", name, 1);
	asprintf(&options->value[1], "%c%c", value, 1);

	return options;
}
*/
static game_entry_t* _createSaveEntry(uint16_t flag, const char* name)
{
	game_entry_t* entry = (game_entry_t *)calloc(1, sizeof(game_entry_t));
	entry->flags = flag;
	entry->name = strdup(name);

	return entry;
}

int set_json_codes(game_entry_t* item)
{
	code_entry_t* cmd;
	item->codes = list_alloc();

//	cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_USER " View Save Details", CMD_VIEW_DETAILS);
//	list_append(item->codes, cmd);

	LOG("Parsing \"%s\"...", item->path);

	char *buffer = readTextFile(item->path, NULL);
	cJSON *cheat = cJSON_Parse(buffer);

	if (!cheat)
	{
		LOG("JSON parse Error: %s\n", buffer);
		list_free(item->codes);
		free(buffer);

		return 0;
	}

	const cJSON *mod;
	const cJSON *mods = cJSON_GetObjectItemCaseSensitive(cheat, "mods");
	const cJSON *proc = cJSON_GetObjectItemCaseSensitive(cheat, "process");
	cJSON_ArrayForEach(mod, mods)
	{
		cJSON *mod_name = cJSON_GetObjectItemCaseSensitive(mod, "name");
		cJSON *mod_type = cJSON_GetObjectItemCaseSensitive(mod, "type");

		if (!cJSON_IsString(mod_name) || !cJSON_IsString(mod_type))
			continue;

		cmd = (code_entry_t *)calloc(1, sizeof(code_entry_t));
		cmd->type = PATCH_VIEW;
		cmd->name = strdup(mod_name->valuestring);
		cmd->file = strdup(cJSON_IsString(proc) ? proc->valuestring : "");
		
		const cJSON *mem = cJSON_GetObjectItemCaseSensitive(mod, "memory");
		char* code = cJSON_Print(mem);
		asprintf(&cmd->codes, "Path: %s\n\nCode: %s\n", item->path, code);
		free(code);

		LOG("Added '%s' (%s)", mod_name->valuestring, mod_type->valuestring);
		list_append(item->codes, cmd);
	}

	cJSON_Delete(cheat);
	free(buffer);

	return list_count(item->codes);
}

static const char* xml_whitespace_cb(mxml_node_t *node, int where)
{
	if (where == MXML_WS_AFTER_OPEN || where == MXML_WS_AFTER_CLOSE)
		return ("\n");

	return (NULL);
}

int set_shn_codes(game_entry_t* item)
{
	code_entry_t* cmd;
	item->codes = list_alloc();

//	cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_USER " View Save Details", CMD_VIEW_DETAILS);
//	list_append(item->codes, cmd);

	LOG("Parsing %s...", item->path);

	char *buffer = readTextFile(item->path, NULL);
	mxml_node_t *node, *tree = NULL;

	/* parse the file and get the DOM */
	tree = mxmlLoadString(NULL, buffer, MXML_NO_CALLBACK);
	if (!tree)
	{
		LOG("XML: could not parse XML: %s", buffer);
		list_free(item->codes);
		free(buffer);

		return 0;
	}

	node = mxmlFindElement(tree, tree, "Trainer", "Game", NULL, MXML_DESCEND);
	const char* author = mxmlElementGetAttr(node, "Moder");
	const char* procfn = mxmlElementGetAttr(node, "Process");

	/* Get the cheat element nodes */
	for (node = mxmlFindElement(tree, tree, "Cheat", "Text", NULL, MXML_DESCEND); node != NULL;
		node = mxmlFindElement(node, tree, "Cheat", "Text", NULL, MXML_DESCEND))
	{
		mxml_node_t *value = mxmlFindElement(node, node, "Cheatline", NULL, NULL, MXML_DESCEND);
		char* code = value ? mxmlSaveAllocString(node, &xml_whitespace_cb) : calloc(1, 1);

		cmd = (code_entry_t *)calloc(1, sizeof(code_entry_t));
		cmd->type = PATCH_VIEW;
		cmd->name = strdup(mxmlElementGetAttr(node, "Text"));
		cmd->file = strdup(procfn);
		asprintf(&cmd->codes, "Path: %s\nAuthor: %s\n\n%s", item->path, author, code);
		free(code);

		LOG("Added '%s' (%d)", cmd->name, cmd->type);
		list_append(item->codes, cmd);
	}

	/* free the document */
	mxmlDelete(tree);
	free(buffer);

	return list_count(item->codes);
}

/*
 * Function:		ReadLocalCodes()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Reads an entire NCL file into an array of code_entry
 * Arguments:
 *	path:			Path to ncl
 *	_count_count:	Pointer to int (set to the number of codes within the ncl)
 * Return:			Returns an array of code_entry, null if failed to load
 */
int ReadCodes(game_entry_t * save)
{
	if (save->flags & CHEAT_FLAG_JSON)
		return set_json_codes(save);

	if (save->flags & CHEAT_FLAG_SHN)
		return set_shn_codes(save);

	save->codes = list_alloc();

	return list_count(save->codes);
}

/*
 * Function:		ReadOnlineSaves()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Downloads an entire NCL file into an array of code_entry
 * Arguments:
 *	filename:		File name ncl
 *	_count_count:	Pointer to int (set to the number of codes within the ncl)
 * Return:			Returns an array of code_entry, null if failed to load
 */
int ReadOnlineSaves(game_entry_t * game)
{
//	code_entry_t* item;
	char path[256];
	snprintf(path, sizeof(path), GOLDCHEATS_LOCAL_CACHE "%s", strrchr(game->path, '/') + 1);

	if (file_exists(path) == SUCCESS)
	{
		struct stat stats;
		stat(path, &stats);
		// re-download if file is +1 day old
		if ((stats.st_mtime + ONLINE_CACHE_TIMEOUT) < time(NULL))
			http_download(game->path, "", path, 0);
	}
	else
	{
		if (!http_download(game->path, "", path, 0))
			return -1;
	}

	char *tmp = game->path;
	game->path = path;

	if (game->flags & CHEAT_FLAG_JSON)
		set_json_codes(game);

	if (game->flags & CHEAT_FLAG_SHN)
		set_shn_codes(game);

	game->path = tmp;

	return (list_count(game->codes));
}

void DownloadChmod(void)
{
	sceKernelChmod(GOLDCHEATS_DATA_PATH, 0777);
	sceKernelChmod(GOLDCHEATS_PATCH_PATH, 0777);
	sceKernelChmod(GOLDCHEATS_LOCAL_CACHE, 0777);
	return;
}
list_t * ReadBackupList(const char* userPath)
{
	DownloadChmod();
	if (extract_zip_gh(USB0_PATH "GoldHEN_Cheat_Repository-main.zip", GOLDCHEATS_DATA_PATH))
	{
		char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
		show_message("Successfully installed offline %s data from USB\n%s", "cheat", cheat_ver);
		free(cheat_ver);
	}
	else if (extract_zip_gh("/data/GoldHEN_Cheat_Repository-main.zip", GOLDCHEATS_DATA_PATH))
	{
		char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
		show_message("Successfully installed offline %s data from HDD\n%s", "cheat", cheat_ver);
		free(cheat_ver);
	}
	if (extract_zip_gh(USB0_PATH "patch1.zip", GOLDCHEATS_PATCH_PATH))
	{
		char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
		show_message("Successfully installed offline %s data from USB\n%s", "patch", patch_ver);
		free(patch_ver);
	}
	else if (extract_zip_gh("/data/patch1.zip", GOLDCHEATS_PATCH_PATH))
	{
		char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
		show_message("Successfully installed offline %s data from HDD\n%s", "patch", patch_ver);
		free(patch_ver);
	}
	if (http_download("https://github.com/GoldHEN/GoldHEN_Cheat_Repository/archive/refs/heads/", "main.zip", GOLDCHEATS_LOCAL_CACHE "appdata.zip", 1))
	{
		int ret = extract_zip_gh(GOLDCHEATS_LOCAL_CACHE "appdata.zip", GOLDCHEATS_DATA_PATH);

		if (ret > 0)
		{
			char *cheat_ver = readTextFile(GOLDCHEATS_DATA_PATH "misc/cheat_ver.txt", NULL);
			show_message("Successfully installed %d %s files\n%s", ret, "cheat", cheat_ver);
			free(cheat_ver);
		}
		else
			show_message("No files extracted!");

		unlink_secure(GOLDCHEATS_LOCAL_CACHE "appdata.zip");
	}

	if (http_download("https://github.com/GoldHEN/GoldHEN_Patch_Repository/raw/gh-pages/", "patch1.zip", GOLDCHEATS_LOCAL_CACHE "appdata.zip", 1))
	{
		int ret = extract_zip_gh(GOLDCHEATS_LOCAL_CACHE "appdata.zip", GOLDCHEATS_PATCH_PATH);
		if (ret > 0)
		{
			char *patch_ver = readTextFile(GOLDCHEATS_PATCH_PATH "misc/patch_ver.txt", NULL);
			show_message("Successfully installed %d %s files\n%s", ret, "patch", patch_ver);
			free(patch_ver);
		}
		else
			show_message("No files extracted!");
		unlink_secure(GOLDCHEATS_LOCAL_CACHE "appdata.zip");
	}
	else show_message("Download error!");

	return NULL;

/*
	char tmp[32];
	game_entry_t * item;
	code_entry_t * cmd;
	list_t *list = list_alloc();

	item = _createSaveEntry(CHEAT_FLAG_PS4, CHAR_ICON_COPY " Export Licenses");
	asprintf(&item->path, EXDATA_PATH_HDD, apollo_config.user_id);
	item->type = FILE_TYPE_RIF;
	list_append(list, item);

	item = _createSaveEntry(CHEAT_FLAG_PS4, CHAR_ICON_USER " Activate PS4 Accounts");
//	asprintf(&item->path, EXDATA_PATH_HDD, apollo_config.user_id);
	item->path = strdup("EXDATA_PATH_HDD");
	item->type = FILE_TYPE_NULL;
	list_append(list, item);

	item = _createSaveEntry(CHEAT_FLAG_PS4, CHAR_ICON_LOCK " Show Parental Security Passcode");
	item->codes = list_alloc();
	cmd = _createCmdCode(PATCH_NULL, CHAR_ICON_LOCK " Security Passcode: ????????", CMD_CODE_NULL);
//	regMgr_GetParentalPasscode(tmp);
	strncpy(cmd->name + 21, tmp, 8);
	list_append(item->codes, cmd);
	list_append(list, item);

	for (int i = 0; i <= MAX_USB_DEVICES; i++)
	{
		snprintf(tmp, sizeof(tmp), USB_PATH, i);

		if (dir_exists(tmp) != SUCCESS)
			continue;

		snprintf(tmp, sizeof(tmp), CHAR_ICON_COPY " Import Licenses (USB %d)", i);
		item = _createSaveEntry(CHEAT_FLAG_PS3, tmp);
		asprintf(&item->path, IMPORT_RAP_PATH_USB, i);
		item->type = FILE_TYPE_RAP;
		list_append(list, item);
	}

	return list;
*/
}

int ReadBackupCodes(game_entry_t * bup)
{
//	code_entry_t * cmd;
	char fext[5] = "";

	switch(bup->type)
	{
	case FILE_TYPE_NULL:
		bup->codes = list_alloc();

//		cmd = _createCmdCode(account ? PATCH_NULL : PATCH_COMMAND, tmp, CMD_CODE_NULL); //CMD_CREATE_ACT_DAT
//		list_append(bup->codes, cmd);

		return list_count(bup->codes);

	default:
		return 0;
	}

	bup->codes = list_alloc();

	LOG("Loading %s files from '%s'...", fext, bup->path);
/*
	if (bup->type == FILE_TYPE_RIF)
	{
		cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_ZIP " Backup All Licenses to .Zip", CMD_CODE_NULL);
		cmd->options_count = 1;
		cmd->options = _createOptions(2, "Save .Zip to USB", CMD_EXP_EXDATA_USB);
		list_append(bup->codes, cmd);

		cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_COPY " Export All Licenses as .RAPs", CMD_CODE_NULL);
		cmd->options_count = 1;
		cmd->options = _createOptions(3, "Save .RAPs to USB", CMD_EXP_LICS_RAPS);
		asprintf(&cmd->options->name[2], "Save .RAPs to HDD");
		asprintf(&cmd->options->value[2], "%c%c", CMD_EXP_LICS_RAPS, 0x10);
		list_append(bup->codes, cmd);
	}

	if (bup->type == FILE_TYPE_RAP)
	{
		cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_COPY " Import All .RAPs as .RIF Licenses", CMD_IMP_EXDATA_USB);
		list_append(bup->codes, cmd);
	}

	DIR *d;
	struct dirent *dir;
	d = opendir(bup->path);

	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0  &&
				endsWith(dir->d_name, fext))
			{
				cmd = _createCmdCode(PATCH_COMMAND, dir->d_name, CMD_CODE_NULL);
				*strrchr(cmd->name, '.') = 0;

				if (bup->type == FILE_TYPE_RIF)
				{
					cmd->options_count = 1;
					cmd->options = _createOptions(3, "Save .RAP to USB", CMD_EXP_LICS_RAPS);
					asprintf(&cmd->options->name[2], "Save .RAP to HDD");
					asprintf(&cmd->options->value[2], "%c%c", CMD_EXP_LICS_RAPS, 0x10);
				}
				else if (bup->type == FILE_TYPE_RAP)
				{
					sprintf(cmd->codes, "%c", CMD_IMP_EXDATA_USB);
				}

				cmd->file = strdup(dir->d_name);
				list_append(bup->codes, cmd);

				LOG("Added File '%s'", cmd->file);
			}
		}
		closedir(d);
	}
*/
	LOG("%d items loaded", list_count(bup->codes));

	return list_count(bup->codes);
}

static int is_patch_enabled(uint64_t hash)
{
	char hash_path[256];
	uint8_t settings[2];

	snprintf(hash_path, sizeof(hash_path), GOLDCHEATS_PATCH_PATH "settings/0x%016lx.txt", hash);

	if(read_file(hash_path, settings, sizeof(settings)) < 0 || settings[0] != 0x31)
		return 0;

	return 1;
}

list_t * ReadPatchList(const char* userPath)
{
	DIR *d;
	list_t* list;
	struct dirent *dir;
	game_entry_t *item;
	char fullPath[256];

	d = opendir(userPath);

	if (!d)
		return NULL;

	list = list_alloc();

	while ((dir = readdir(d)) != NULL)
	{
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0 || endsWith(dir->d_name, ".json") == NULL)
			continue;

		snprintf(fullPath, sizeof(fullPath), "%s%s", userPath, dir->d_name);
		if (file_exists(fullPath) != SUCCESS)
			continue;

		LOG("Reading %s...", fullPath);

		char *ver = fullPath;
		char *app = fullPath;
		char *buffer = readTextFile(fullPath, NULL);
		cJSON *cheat = cJSON_Parse(buffer);

		if (!cheat)
		{
			LOG("JSON parse Error: %s", buffer);
			free(buffer);
			continue;
		}

		const cJSON *game;
		const cJSON *patchs = cJSON_GetObjectItemCaseSensitive(cheat, "patch");
		cJSON_ArrayForEach(game, patchs)
		{
			const cJSON *app_name = cJSON_GetObjectItemCaseSensitive(game, "title");
			const cJSON *app_ver = cJSON_GetObjectItemCaseSensitive(game, "app_ver");

			if (!cJSON_IsString(app_name) || !cJSON_IsString(app_ver) || 
				(strcmp(app_name->valuestring, app) == 0 && strcmp(app_ver->valuestring, ver) == 0))
				continue;

			ver = app_ver->valuestring;
			app = app_name->valuestring;
			item = _createSaveEntry(CHEAT_FLAG_PS4 | CHEAT_FLAG_HDD | CHEAT_FLAG_PATCH, app_name->valuestring);
			item->type = FILE_TYPE_PS4;
			item->path = strdup(fullPath);
			item->version = strdup(app_ver->valuestring);
			item->title_id = strdup(dir->d_name);
			*strrchr(item->title_id, '.') = 0;

			LOG("[%s] F(%X) '%s' %s", item->title_id, item->flags, item->name, item->version);
			list_append(list, item);
		}

		cJSON_Delete(cheat);
		free(buffer);
	}
	
	closedir(d);

	LOG("%d items loaded", list_count(list));
	if (!list_count(list))
	{
		list_free(list);
		return NULL;
	}

	check_game_appdb(list);
	return list;
}

int ReadPatches(game_entry_t * game)
{
	code_entry_t* cmd;
	game->codes = list_alloc();

	LOG("Loading patches from '%s'...", game->path);

	char *buffer = readTextFile(game->path, NULL);
	cJSON *cheat = cJSON_Parse(buffer);

	if (!cheat)
	{
		LOG("JSON parse Error: %s", buffer);
		list_free(game->codes);
		free(buffer);

		return 0;
	}

	const cJSON *app;
	const cJSON *patchs = cJSON_GetObjectItemCaseSensitive(cheat, "patch");
	cJSON_ArrayForEach(app, patchs)
	{
		const cJSON *app_ver = cJSON_GetObjectItemCaseSensitive(app, "app_ver");
		const cJSON *app_name = cJSON_GetObjectItemCaseSensitive(app, "title");

		if (!cJSON_IsString(app_ver) || !cJSON_IsString(app_name) || 
			strcmp(app_name->valuestring, game->name) != 0 || strcmp(app_ver->valuestring, game->version) != 0)
			continue;

		const cJSON *obj = cJSON_GetObjectItemCaseSensitive(app, "name");
		cmd = (code_entry_t *)calloc(1, sizeof(code_entry_t));
		cmd->type = PATCH_VIEW;
		cmd->name = strdup(cJSON_IsString(obj) ? obj->valuestring : "");

		obj = cJSON_GetObjectItemCaseSensitive(app, "app_elf");
		cmd->file = strdup(cJSON_IsString(obj) ? obj->valuestring : "");

		obj = cJSON_GetObjectItemCaseSensitive(app, "note");
		char* note = cJSON_IsString(obj) ? obj->valuestring : "";

		obj = cJSON_GetObjectItemCaseSensitive(app, "author");
		char* author = cJSON_IsString(obj) ? obj->valuestring : "";

		obj = cJSON_GetObjectItemCaseSensitive(app, "patch_list");
		char* code = cJSON_Print(obj);
		cmd->activated = is_patch_enabled(patch_hash_calc(game, cmd));

		asprintf(&cmd->codes, "Game Title: %s\n"
			"Game Version: %s\n"
			"Patch Name: %s\n"
			"Patch Author: %s\n"
			"Patch Elf: %s\n"
			"Patch Path: %s\n"
			"Patch Note: %s\n"
			"Patch Code: %s\n",
			game->name, game->version, cmd->name, author, cmd->file, game->path, note, code);
		free(code);

		list_append(game->codes, cmd);
		LOG("Added '%s' (%s)", cmd->name, cmd->file);
	}

	cJSON_Delete(cheat);
	free(buffer);

	LOG("%d items loaded", list_count(game->codes));

	return list_count(game->codes);
}

/*
 * Function:		UnloadGameList()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Free entire array of game_entry
 * Arguments:
 *	list:			Array of game_entry to free
 *	count:			number of game entries
 * Return:			void
 */
void UnloadGameList(list_t * list)
{
	list_node_t *node, *nc;
	game_entry_t *item;
	code_entry_t *code;

	for (node = list_head(list); (item = list_get(node)); node = list_next(node))
	{
		if (item->name)
		{
			free(item->name);
			item->name = NULL;
		}

		if (item->path)
		{
			free(item->path);
			item->path = NULL;
		}

		if (item->version)
		{
			free(item->version);
			item->version = NULL;
		}

		if (item->title_id)
		{
			free(item->title_id);
			item->title_id = NULL;
		}
		
		if (item->codes)
		{
			for (nc = list_head(item->codes); (code = list_get(nc)); nc = list_next(nc))
			{
				if (code->codes)
				{
					free (code->codes);
					code->codes = NULL;
				}
				if (code->name)
				{
					free (code->name);
					code->name = NULL;
				}
				if (code->options && code->options_count > 0)
				{
					for (int z = 0; z < code->options_count; z++)
					{
						if (code->options[z].line)
							free(code->options[z].line);
						if (code->options[z].name)
							free(code->options[z].name);
						if (code->options[z].value)
							free(code->options[z].value);
					}
					
					free (code->options);
				}
			}
			
			list_free(item->codes);
			item->codes = NULL;
		}
	}

	list_free(list);
	
	LOG("UnloadGameList() :: Successfully unloaded game list");
}

int sortCodeList_Compare(const void* a, const void* b)
{
	return strcasecmp(((code_entry_t*) a)->name, ((code_entry_t*) b)->name);
}

int sortGameList_Exists(const void* a, const void* b)
{
	return ((((game_entry_t*) a)->flags & CHEAT_FLAG_OWNER) < (((game_entry_t*) b)->flags & CHEAT_FLAG_OWNER));
}

/*
 * Function:		qsortSaveList_Compare()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Compares two game_entry for QuickSort
 * Arguments:
 *	a:				First code
 *	b:				Second code
 * Return:			1 if greater, -1 if less, or 0 if equal
 */
int sortGameList_Compare(const void* a, const void* b)
{
	return strcasecmp(((game_entry_t*) a)->name, ((game_entry_t*) b)->name);
}

static void read_shn_games(const char* userPath, const char* fext, list_t *list)
{
	DIR *d;
	struct dirent *dir;
	game_entry_t *item;
	char fullPath[256];

	d = opendir(userPath);
	if (!d)
		return;

	while ((dir = readdir(d)) != NULL)
	{
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0 || endsWith(dir->d_name, fext) == NULL)
			continue;

		snprintf(fullPath, sizeof(fullPath), "%s%s", userPath, dir->d_name);
		if (file_exists(fullPath) != SUCCESS)
			continue;

		LOG("Reading %s...", dir->d_name);

		char *buffer = readTextFile(fullPath, NULL);
		mxml_node_t *node, *tree = NULL;

		/* parse the file and get the DOM */
		tree = mxmlLoadString(NULL, buffer, MXML_NO_CALLBACK);
		if (!tree)
		{
			LOG("XML: could not parse XML: %s", buffer);
			free(buffer);
			continue;
		}

		node = mxmlFindElement(tree, tree, "Trainer", "Game", NULL, MXML_DESCEND);
		if (!node)
		{
			LOG("Invalid SHN file %s", fullPath);
			free(buffer);
			continue;
		}

		item = _createSaveEntry(CHEAT_FLAG_PS4 | CHEAT_FLAG_HDD, mxmlElementGetAttr(node, "Game"));
		item->type = FILE_TYPE_PS4;
		item->path = strdup(fullPath);
		item->version = strdup(mxmlElementGetAttr(node, "Version"));
		item->title_id = strdup(mxmlElementGetAttr(node, "Cusa"));
		item->flags |= CHEAT_FLAG_SHN;

		/* free the document */
		mxmlDelete(tree);
		free(buffer);

		LOG("[%s] F(%d) '%s'", item->title_id, item->flags, item->name);
		list_append(list, item);
	}
	
	closedir(d);
	return;
}

static void read_json_games(const char* userPath, list_t *list)
{
	DIR *d;
	struct dirent *dir;
	game_entry_t *item;
	char fullPath[256];

	d = opendir(userPath);

	if (!d)
		return;

	while ((dir = readdir(d)) != NULL)
	{
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0 || endsWith(dir->d_name, ".json") == NULL)
			continue;

		snprintf(fullPath, sizeof(fullPath), "%s%s", userPath, dir->d_name);
		if (file_exists(fullPath) != SUCCESS)
			continue;

		LOG("Reading %s...", dir->d_name);

		char *buffer = readTextFile(fullPath, NULL);
		cJSON *cheat = cJSON_Parse(buffer);
		const cJSON *jobject;

		if (!cheat)
		{
			LOG("JSON parse Error: %s\n", buffer);
			free(buffer);
			continue;
		}

		jobject = cJSON_GetObjectItemCaseSensitive(cheat, "name");
		item = _createSaveEntry(CHEAT_FLAG_PS4 | CHEAT_FLAG_HDD, cJSON_IsString(jobject) ? jobject->valuestring : "");
		item->type = FILE_TYPE_PS4;
		item->path = strdup(fullPath);

		jobject = cJSON_GetObjectItemCaseSensitive(cheat, "version");
		item->version = strdup(cJSON_IsString(jobject) ? jobject->valuestring : "");

		jobject = cJSON_GetObjectItemCaseSensitive(cheat, "id");
		item->title_id = strdup(cJSON_IsString(jobject) ? jobject->valuestring : "");
		item->flags |= CHEAT_FLAG_JSON;

		cJSON_Delete(cheat);
		free(buffer);

		LOG("[%s] F(%d) '%s'", item->title_id, item->flags, item->name);
		list_append(list, item);
	}
	
	closedir(d);
	return;
}

/*
 * Function:		ReadUserList()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Reads the entire userlist folder into a game_entry array
 * Arguments:
 *	gmc:			Set as the number of games read
 * Return:			Pointer to array of game_entry, null if failed
 */
list_t * ReadUserList(const char* userPath)
{
//	game_entry_t *item;
//	code_entry_t *cmd;
	list_t *list;
	char fullPath[256];

	if (file_exists(userPath) != SUCCESS)
		return NULL;

	list = list_alloc();

/*
	item = _createSaveEntry(CHEAT_FLAG_PS4, CHAR_ICON_COPY " Bulk Cheats Management");
	item->type = FILE_TYPE_MENU;
	item->codes = list_alloc();
	item->path = strdup(userPath);
//	item->path = (void*) list; //bulk management hack

	cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_SIGN " Resign & Unlock all Saves", CMD_RESIGN_ALL_SAVES);
	list_append(item->codes, cmd);

	cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_COPY " Copy all Saves to USB", CMD_CODE_NULL);
	cmd->options_count = 1;
	cmd->options = _createOptions(2, "Copy Saves to USB", CMD_COPY_SAVES_USB);
	list_append(item->codes, cmd);

	cmd = _createCmdCode(PATCH_COMMAND, CHAR_ICON_LOCK " Dump all Save Fingerprints", CMD_DUMP_FINGERPRINTS);
	list_append(item->codes, cmd);
	list_append(list, item);
*/

	LOG("Loading JSON files...");
	snprintf(fullPath, sizeof(fullPath), "%sjson/", userPath);
	read_json_games(fullPath, list);

	LOG("Loading SHN files...");
	snprintf(fullPath, sizeof(fullPath), "%sshn/", userPath);
	read_shn_games(fullPath, ".shn", list);

	LOG("Loading MC4 files...");
	snprintf(fullPath, sizeof(fullPath), "%smc4/", userPath);
	read_shn_games(fullPath, ".xml", list);

	if (!list_count(list))
	{
		list_free(list);
		return NULL;
	}

	check_game_appdb(list);
	return list;
}

/*
 * Function:		ReadOnlineList()
 * File:			saves.c
 * Project:			Apollo PS3
 * Description:		Downloads the entire gamelist file into a game_entry array
 * Arguments:
 *	gmc:			Set as the number of games read
 * Return:			Pointer to array of game_entry, null if failed
 */
static void _ReadOnlineListEx(const char* urlPath, const char* fext, uint16_t flag, list_t *list)
{
	game_entry_t *item;
	char path[256];
	char fname[64];
	struct stat stats;

	snprintf(path, sizeof(path), GOLDCHEATS_LOCAL_CACHE "%s_games.txt", fext);
	snprintf(fname, sizeof(fname), "%s.txt", fext);

	if (stat(path, &stats) != SUCCESS && !http_download(urlPath, fname, path, 0))
		return;

	// re-download if file is +1 day old
	if ((stats.st_mtime + ONLINE_CACHE_TIMEOUT) < time(NULL) || stats.st_size == 0)
		if (!http_download(urlPath, fname, path, 0))
			return;

	long fsize;
	char *data = readTextFile(path, &fsize);
	
	char *ptr = data;
	char *end = data + fsize;

	while (ptr < end && *ptr)
	{
		char *tmp, *content = ptr;

		while (ptr < end && *ptr != '\n' && *ptr != '\r')
		{
			ptr++;
		}
		*ptr++ = 0;

		if ((tmp = strchr(content, '=')) != NULL)
		{
			*tmp++ = 0;
			item = _createSaveEntry(flag | CHEAT_FLAG_ONLINE, tmp);
			asprintf(&item->path, "%s%s/%s", urlPath, fext, content);

			tmp = strchr(content, '_');
			*tmp++ = 0;
			item->title_id = strdup(content);

			*(tmp + 5) = 0;
			item->version = strdup(tmp);

			LOG("+ [%s %s] %s", item->title_id, item->version, item->name);
			list_append(list, item);
		}

		if (ptr < end && *ptr == '\r')
		{
			ptr++;
		}
		if (ptr < end && *ptr == '\n')
		{
			ptr++;
		}
	}

	if (data) free(data);
}

list_t * ReadOnlineList(const char* urlPath)
{
	list_t *list = list_alloc();

	// PS4 JSON games
	_ReadOnlineListEx(urlPath, "json", CHEAT_FLAG_PS4 | CHEAT_FLAG_JSON, list);

	// PS4 SHN games
	_ReadOnlineListEx(urlPath, "shn", CHEAT_FLAG_PS4 | CHEAT_FLAG_SHN, list);

	// PS4 MC4 games
	_ReadOnlineListEx(urlPath, "mc4", CHEAT_FLAG_PS4 | CHEAT_FLAG_SHN, list);

	if (!list_count(list))
	{
		list_free(list);
		return NULL;
	}

	check_game_appdb(list);
	return list;
}

int get_save_details(const game_entry_t* save, char **details)
{
/*
	char sfoPath[256];
	sqlite3 *db;
	sqlite3_stmt *res;


	if (!(save->flags & CHEAT_FLAG_PS4))
	{
		asprintf(details, "%s\n\nTitle: %s\n", save->path, save->name);
		return 1;
	}

	if (save->flags & CHEAT_FLAG_TROPHY)
	{
		if ((db = open_sqlite_db(save->path)) == NULL)
			return 0;

		char* query = sqlite3_mprintf("SELECT id, description, trophy_num, unlocked_trophy_num, progress,"
			"platinum_num, unlocked_platinum_num, gold_num, unlocked_gold_num, silver_num, unlocked_silver_num,"
			"bronze_num, unlocked_bronze_num FROM tbl_trophy_title WHERE id = %d", save->blocks);

		if (sqlite3_prepare_v2(db, query, -1, &res, NULL) != SQLITE_OK || sqlite3_step(res) != SQLITE_ROW)
		{
			LOG("Failed to fetch data: %s", sqlite3_errmsg(db));
			sqlite3_free(query);
			sqlite3_close(db);
			return 0;
		}

		asprintf(details, "Trophy-Set Details\n\n"
			"Title: %s\n"
			"Description: %s\n"
			"NP Comm ID: %s\n"
			"Progress: %d/%d - %d%%\n"
			"%c Platinum: %d/%d\n"
			"%c Gold: %d/%d\n"
			"%c Silver: %d/%d\n"
			"%c Bronze: %d/%d\n",
			save->name, sqlite3_column_text(res, 1), save->title_id,
			sqlite3_column_int(res, 3), sqlite3_column_int(res, 2), sqlite3_column_int(res, 4),
			CHAR_TRP_PLATINUM, sqlite3_column_int(res, 6), sqlite3_column_int(res, 5),
			CHAR_TRP_GOLD, sqlite3_column_int(res, 8), sqlite3_column_int(res, 7),
			CHAR_TRP_SILVER, sqlite3_column_int(res, 10), sqlite3_column_int(res, 9),
			CHAR_TRP_BRONZE, sqlite3_column_int(res, 12), sqlite3_column_int(res, 11));

		sqlite3_free(query);
		sqlite3_finalize(res);
		sqlite3_close(db);

		return 1;
	}

	if(save->flags & CHEAT_FLAG_LOCKED)
	{
		asprintf(details, "%s\n\n"
			"Title ID: %s\n"
			"Dir Name: %s\n"
			"Blocks: %d\n"
			"Account ID: %.16s\n",
			save->path,
			save->title_id,
			save->version,
			save->blocks,
			save->path + 23);

		return 1;
	}

	if(save->flags & CHEAT_FLAG_HDD)
	{
		if ((db = open_sqlite_db(save->path)) == NULL)
			return 0;

		char* query = sqlite3_mprintf("SELECT sub_title, detail, free_blocks, size_kib, user_id, account_id FROM savedata "
			" WHERE title_id = %Q AND dir_name = %Q", save->title_id, save->version);

		if (sqlite3_prepare_v2(db, query, -1, &res, NULL) != SQLITE_OK || sqlite3_step(res) != SQLITE_ROW)
		{
			LOG("Failed to fetch data: %s", sqlite3_errmsg(db));
			sqlite3_free(query);
			sqlite3_close(db);
			return 0;
		}

		asprintf(details, "%s\n\n"
			"Title: %s\n"
			"Subtitle: %s\n"
			"Detail: %s\n"
			"Dir Name: %s\n"
			"Blocks: %d (%d Free)\n"
			"Size: %d Kb\n"
			"User ID: %08x\n"
			"Account ID: %016llx\n",
			save->path, save->name, 
			sqlite3_column_text(res, 0),
			sqlite3_column_text(res, 1),
			save->version,
			save->blocks, sqlite3_column_int(res, 2), 
			sqlite3_column_int(res, 3),
			sqlite3_column_int(res, 4),
			sqlite3_column_int64(res, 5));

		sqlite3_free(query);
		sqlite3_finalize(res);
		sqlite3_close(db);

		return 1;
	}

	snprintf(sfoPath, sizeof(sfoPath), "%s" "sce_sys/param.sfo", save->path);
	LOG("Save Details :: Reading %s...", sfoPath);

	sfo_context_t* sfo = sfo_alloc();
	if (sfo_read(sfo, sfoPath) < 0) {
		LOG("Unable to read from '%s'", sfoPath);
		sfo_free(sfo);
		return 0;
	}

	char* subtitle = (char*) sfo_get_param_value(sfo, "SUBTITLE");
	char* detail = (char*) sfo_get_param_value(sfo, "DETAIL");
	uint64_t* account_id = (uint64_t*) sfo_get_param_value(sfo, "ACCOUNT_ID");
	sfo_params_ids_t* param_ids = (sfo_params_ids_t*) sfo_get_param_value(sfo, "PARAMS");

	asprintf(details, "%s\n\n"
		"Title: %s\n"
		"Subtitle: %s\n"
		"Detail: %s\n"
		"Dir Name: %s\n"
		"Blocks: %d\n"
		"User ID: %08x\n"
		"Account ID: %016lx\n",
		save->path, save->name,
		subtitle,
		detail,
		save->version,
		save->blocks,
		param_ids->user_id,
		*account_id);

	sfo_free(sfo);
*/
	asprintf(details, "%s\n", save->path);
	return 1;
}
