/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the License);
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an AS IS BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */




#include "common.h"
#include "cbhm_main.h"
#include "storage.h"

/*
   file structure

   /---------------------------------------------------------------------------
    |header|current_position|total_count|item_header|item_body(512kib)|item...|
    --------------------------------------------------------------------------/

*/

#define STORAGE_FILEPATH "/opt/var/.savecbh"
#define STORAGE_MAX_ITEMS HISTORY_QUEUE_MAX_ITEMS
#define HEADER_ITEM_SIZE (sizeof(int))
#define BODY_ITEM_SIZE HISTORY_QUEUE_ITEM_SIZE
#define STORAGE_HEADER_SIZE (STORAGE_MAX_ITEMS * HEADER_ITEM_SIZE)
#define STORAGE_BODY_SIZE (STORAGE_MAX_ITEMS * BODY_ITEM_SIZE)
#define TOTAL_STORAGE_SIZE (STORAGE_HEADER_SIZE+STORAGE_BODY_SIZE)

#define GET_HEADER_ADDR_BY_POSITION(pos) (STORAGE_HEADER_SIZE+pos*(HEADER_ITEM_SIZE+BODY_ITEM_SIZE))
#define GET_BODY_ADDR_BY_POSITION(pos) (GET_HEADER_ADDR_BY_POSITION(pos)+HEADER_ITEM_SIZE)

static FILE *g_storage_file = NULL;
static unsigned int g_storage_serial_number = 0;

int init_storage(void *data)
{
	struct appdata *ad = data;

	int i;
	int result = 0;

	if (g_storage_file)
		return 1;

	g_storage_file = fopen(STORAGE_FILEPATH, "r+");
	if (!g_storage_file)
	{  // making data savefile
		g_storage_file = fopen(STORAGE_FILEPATH, "w+");

		if (!g_storage_file)
		{
			close_storage(ad);
			DTRACE("Error : failed openning file for writing\n");
			return -1;
		}

		result = fseek(g_storage_file, TOTAL_STORAGE_SIZE-1, SEEK_SET);
		if (!result)
		{
			close_storage(ad);
			DTRACE("Error : failed moving file position to file's end\n");
			return -1;
		}

		result = fputc(0, g_storage_file);
		if (result == EOF)
		{
		DTRACE("Error : failed writing to file's end\n");
		return -1;
		}
	}

	DTRACE("Success : storage init is done\n");

	g_storage_serial_number = 0;

	return 0;
}

int sync_storage(void *data)
{
//	struct appdata *ad = data;

	if (!g_storage_file)
	{
		DTRACE("g_storage_file is null\n");
		return -1;
	}
	fsync(g_storage_file);

	return 0;
}

unsigned int get_storage_serial_code(void *data)
{
//	struct appdata *ad = data;

	return g_storage_serial_number;
}

int adding_item_to_storage(void *data, int pos, char *idata)
{
//	struct appdata *ad = data;
	if (!g_storage_file)
	{
		DTRACE("g_storage_file is null\n");
		return -1;
	}

	int result;
	result = fseek(g_storage_file, GET_HEADER_ADDR_BY_POSITION(pos), SEEK_SET);
	// FIXME : replace from fprintf to fwrite
	fprintf(g_storage_file, "%d", strlen(idata));
	fprintf(g_storage_file, "%s", idata);

	g_storage_serial_number++;
	return 0;
}

int get_item_counts(void *data)
{
	struct appdata *ad = data;

	return ad->hicount;
}

int close_storage(void *data)
{
	struct appdata *ad = data;

	if (g_storage_file)
		fclose(g_storage_file);
	g_storage_file = NULL;
	g_storage_serial_number = 0;

	return 0;
}

int check_regular_file(char *path)
{
	struct stat fattr;
	if (stat(path, &fattr))
	{
		DTRACE("Cannot get file at path = %s\n", path);
		return FALSE;
	}

	return S_ISREG(fattr.st_mode);
}

