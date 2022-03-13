#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <orbis/SaveData.h>

#include "cheats.h"
#include "menu.h"
#include "common.h"
#include "util.h"


void execCodeCommand(code_entry_t* code, const char* codecmd)
{
	switch (codecmd[0])
	{
		case CMD_DECRYPT_FILE:
//			decryptSaveFile(code->options[0].name[code->options[0].sel]);
			code->activated = 0;
			break;

		default:
			break;
	}

	return;
}
