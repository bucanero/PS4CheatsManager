#include <zip.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>

#include "cheats.h"
#include "common.h"
#include "settings.h"


void walk_zip_directory(const char* startdir, const char* inputdir, struct zip_t *zipper)
{
	char fullname[256];	
	struct dirent *dirp;
	int len = strlen(startdir) + 1;
	DIR *dp = opendir(inputdir);

	if (!dp) {
		LOG("Failed to open input directory: '%s'", inputdir);
		return;
	}

	if (strlen(inputdir) > len)
	{
		LOG("Adding folder '%s'", inputdir+len);
/*
		if (zip_add_dir(zipper, inputdir+len) < 0)
		{
			LOG("Failed to add directory to zip: %s", inputdir);
			return;
		}
*/
	}

	while ((dirp = readdir(dp)) != NULL) {
		if ((strcmp(dirp->d_name, ".")  != 0) && (strcmp(dirp->d_name, "..") != 0)) {
			snprintf(fullname, sizeof(fullname), "%s%s", inputdir, dirp->d_name);

			if (dirp->d_type == DT_DIR) {
				strcat(fullname, "/");
				walk_zip_directory(startdir, fullname, zipper);
			} else {
				LOG("Adding file '%s'", fullname+len);

				zip_entry_open(zipper, fullname+len);
				if (zip_entry_fwrite(zipper, fullname) != 0) {
					LOG("Failed to add file to zip: %s", fullname);
				}
				zip_entry_close(zipper);
			}
		}
	}
	closedir(dp);
}

int zip_directory(const char* basedir, const char* inputdir, const char* output_filename)
{
    struct zip_t *archive = zip_open(output_filename, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

    if (!archive) {
        LOG("Failed to open output file '%s'", output_filename);
        return 0;
    }

    LOG("Zipping <%s> to %s...", inputdir, output_filename);
    init_loading_screen("Creating archive...");
    walk_zip_directory(basedir, inputdir, archive);
    zip_close(archive);
    stop_loading_screen();

    return (file_exists(output_filename) == SUCCESS);
}

int on_extract_entry(const char *filename, void *arg)
{
	uint64_t* progress = (uint64_t*) arg;

    LOG("Extracted: %s", filename);
    update_progress_bar(++progress[0], progress[1], filename);

    return 0;
}

int extract_zip(const char* zip_file, const char* dest_path)
{
	int ret;
	uint64_t progress[2];
	struct zip_t *archive = zip_open(zip_file, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r');

	if (!archive)
		return 0;

	progress[0] = 0;
	progress[1] = zip_entries_total(archive);
	zip_close(archive);

	LOG("Extracting ZIP (%d) to <%s>...", progress[1], dest_path);

	init_progress_bar("Extracting files...");
	ret = zip_extract(zip_file, dest_path, on_extract_entry, progress);
	end_progress_bar();

	return (ret == SUCCESS);
}

int extract_zip_gh(const char* zip_file, const char* dest_path)
{
	int file_entries, ret = 0;
	const char *name = NULL;
	struct zip_t *zip = zip_open(zip_file, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r');

	if (!zip)
	{
		LOG("Zip file %s is null", zip_file);
		return 0;
	}

	file_entries = zip_entries_total(zip);
	init_progress_bar("Extracting files...");

	for (int i = 0; i < file_entries; ++i)
	{
		char fpath[256] = {0};
		zip_entry_openbyindex(zip, i);
		name = strchr(zip_entry_name(zip), '/');

		LOG("zip_entry_name %s | strchr %s", zip_entry_name(zip), name);
		if (!zip_entry_isdir(zip) && name &&
			!(startsWith(name, "/json/") &&
			startsWith(name, "/xml/") &&
			startsWith(name, "/shn/") &&
			startsWith(name, "/mc4/") &&
			startsWith(name, "/misc/")))
		{
			snprintf(fpath, sizeof(fpath), "%s%s", dest_path, name);
		}
		else if (!zip_entry_isdir(zip) && name &&
			!(startsWith(zip_entry_name(zip), "plugins/") &&
			  startsWith(zip_entry_name(zip), "json/") &&
			  startsWith(zip_entry_name(zip), "shn/") &&
			  startsWith(zip_entry_name(zip), "mc4/") &&
			  startsWith(zip_entry_name(zip), "xml/") &&
			  startsWith(zip_entry_name(zip), "misc/")))
		{
			snprintf(fpath, sizeof(fpath), "%s/%s", dest_path, zip_entry_name(zip));
		}
		else
		{
			LOG("Not valid path: %s", zip_entry_name(zip));
			zip_entry_close(zip);
			continue;
		}

		if (!gcm_config.overwrite && file_exists(fpath) == SUCCESS)
		{
			zip_entry_close(zip);
			continue;
		}

		if (!startsWith(fpath, "/data/") ||
			!startsWith(fpath, "/mnt/"))
		{
			LOG("Extracting %s (%i/%i)", fpath, i, file_entries);
			mkdirs(fpath);
			update_progress_bar(i, file_entries, "Extracting files...");
			ret += (zip_entry_fread(zip, fpath) == SUCCESS);
		}
		zip_entry_close(zip);
	}

	end_progress_bar();
	zip_close(zip);

	return (ret);
}
