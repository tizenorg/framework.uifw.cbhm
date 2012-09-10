/*
 * Copyright 2012  Samsung Electronics Co., Ltd

 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.tizenopensource.org/license

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <Ecore_File.h>

#include "storage.h"
#define STORAGE_FILEPATH "/opt/var/.savecbh"
#define STORAGE_KEY_INDEX "index"
#define STORAGE_KEY_FORMAT "data%02d"
#define STORAGE_INDEX_ITEM_NONE 0.0

static void storage_item_index_wrote(StorageData *sd);
static Eina_Bool item_write(Eet_File *ef, int index, CNP_ITEM *item);
static void storage_rewrite_all_items(StorageData *sd);
static Eina_Bool storage_index_write(StorageData *sd);
static Eina_Bool storage_item_write(AppData *ad, CNP_ITEM *item);
static Eina_Bool storage_item_delete(AppData *ad, CNP_ITEM *item);
static CNP_ITEM *storage_item_load(StorageData *sd, int index);
#ifdef DEBUG
static void dump_items(StorageData *sd);
#else
#define dump_items(a)
#endif

static int getMinIndex(indexType *indexTable, int len)
{
	int i = 0;
	int minIndex;
	indexType min;
	min = indexTable[i];
	minIndex = i;

	for (i = 1; i < len; i++)
	{
		if ((min > indexTable[i]))
		{
			min = indexTable[i];
			minIndex = i;
		}
	}
	return minIndex;
}

static int getMaxIndex(indexType *indexTable, int len)
{
	int i = 0;
	indexType max = indexTable[i];
	int maxIndex = i;
	for (i = 1; i < len; i++)
	{
		if (max < indexTable[i])
		{
			max = indexTable[i];
			maxIndex = i;
		}
	}
	return maxIndex;
}

StorageData *init_storage(AppData *ad)
{
	CALLED();
	StorageData *sd = CALLOC(1, sizeof(StorageData));
	eet_init();
	ecore_file_init();

	sd->ef = eet_open(STORAGE_FILEPATH, EET_FILE_MODE_READ_WRITE);
	/*
	if (sd->ef)
	{
		int read_size;
		indexType *read_data;
		read_data = eet_read(sd->ef, STORAGE_KEY_INDEX, &read_size);

		int storage_size = sizeof(indexType) * STORAGE_ITEM_CNT;

		int copy_size = storage_size < read_size ? storage_size : read_size;

		if (read_data)
		{
			indexType *temp = MALLOC(read_size);
			if (!temp)
				return sd;
			memcpy(temp, read_data, read_size);

			int i;
			int copy_cnt = copy_size/sizeof(indexType);
			for (i = 0; i < copy_cnt; i++)
			{
				int maxIndex = getMaxIndex(temp, copy_cnt);
				if (temp[maxIndex] == STORAGE_INDEX_ITEM_NONE)
					break;
				sd->itemTable[i] = storage_item_load(sd, maxIndex);
				if (sd->itemTable[i])
					sd->indexTable[i] = temp[maxIndex];
				temp[maxIndex] = STORAGE_INDEX_ITEM_NONE;

				DMSG("load storage item index %d\n", i);
			}
			for (i = copy_cnt - 1; i >= 0; i--)
			{
				if (sd->itemTable[i])
					item_add_by_CNP_ITEM(ad, sd->itemTable[i]);
			}
		}
		else
		{
			DMSG("load storage index failed\n");
		}
	}
	else
		DMSG("storage ef is NULLd\n");
	*/
	dump_items(sd);

	ad->storage_item_add = storage_item_write;
	ad->storage_item_del = storage_item_delete;
//	ad->storage_item_load = storage_item_load;

	return sd;
}

void depose_storage(StorageData *sd)
{
	CALLED();
	storage_rewrite_all_items(sd);
	dump_items(sd);
	if (sd->ef)
		eet_close(sd->ef);
	sd->ef = NULL;
	eet_shutdown();
	ecore_file_shutdown();
}

#ifdef DEBUG
static void dump_items(StorageData *sd)
{
	CALLED();
	int i;
	for (i = 0; i < STORAGE_ITEM_CNT; i++)
	{
		CNP_ITEM *item = storage_item_load(sd, i);
		if (item)
			printf("item #%d type: 0x%x, data: %s\n, len: %d\n", i, item->type_index, item->data, item->len);
	}
}
#endif

static Eina_Bool item_write(Eet_File *ef, int index, CNP_ITEM *item)
{
	if (!ef)
	{
		DMSG("eet_file is NULL\n");
		return EINA_FALSE;
	}
	char datakey[10];
	snprintf(datakey, 10, STORAGE_KEY_FORMAT, index);
	int buf_size = item->len + sizeof(int);
	char *buf = MALLOC(buf_size);
	if (!buf)
		return EINA_FALSE;
	((int *)buf)[0] = item->type_index;
	char *data = buf + sizeof(int);
	memcpy(data, item->data, item->len);

	int ret = eet_write(ef, datakey, buf, buf_size, 1);
	DMSG("write result: %d, datakey: %s, buf_size: %d, item_len: %d\n", ret, datakey, buf_size, item->len);
/*	if (ret)
		eet_sync(ef);*/
	return ret != 0;
}

static void storage_rewrite_all_items(StorageData *sd)
{
	CALLED();
	if (sd->ef)
		eet_close(sd->ef);
	ecore_file_remove(STORAGE_FILEPATH);
	sd->ef = eet_open(STORAGE_FILEPATH, EET_FILE_MODE_READ_WRITE);

	int i;
	for (i = 0; i < STORAGE_ITEM_CNT; i++)
	{
		if ((sd->indexTable[i] != STORAGE_INDEX_ITEM_NONE) && (sd->itemTable[i]))
			item_write(sd->ef, i, sd->itemTable[i]);
	}
	storage_index_write(sd);
}

static Eina_Bool storage_item_write(AppData *ad, CNP_ITEM *item)
{
	CALLED();
	StorageData *sd = ad->storage;
	int index = getMinIndex(sd->indexTable, STORAGE_ITEM_CNT);
	sd->indexTable[index] = ecore_time_unix_get();
	sd->itemTable[index] = item;

	item_write(sd->ef, index, item);
	storage_index_write(sd);
	dump_items(sd);
	return EINA_TRUE;
}

static Eina_Bool storage_item_delete(AppData *ad, CNP_ITEM *item)
{
	CALLED();
	StorageData *sd = ad->storage;
	int index;
	for (index = 0; index < STORAGE_ITEM_CNT; index++)
	{
		if (sd->itemTable[index] == item)
			break;
	}

	if (index < STORAGE_ITEM_CNT)
	{
		sd->indexTable[index] = STORAGE_INDEX_ITEM_NONE;
		storage_index_write(sd);
	}
	return EINA_TRUE;
}

static CNP_ITEM *storage_item_load(StorageData *sd, int index)
{
	if (!sd->ef)
	{
		DMSG("eet_file is NULL\n");
		return EINA_FALSE;
	}
	if (index >= STORAGE_ITEM_CNT)
		return NULL;

	indexType copyTable[STORAGE_ITEM_CNT];
	memcpy(copyTable, sd->indexTable, sizeof(copyTable));
	int i;
	for (i = 0; i < index; i++)
	{
		int maxIndex = getMaxIndex(copyTable, STORAGE_ITEM_CNT);
		if (maxIndex == -1)
			maxIndex = 0;
		copyTable[maxIndex] = 0;
	}

	char datakey[10];
	snprintf(datakey, 10, STORAGE_KEY_FORMAT, i);

	int read_size;
	char *read_data = eet_read(sd->ef, datakey, &read_size);

	if (!read_data)
	{
		DMSG("read failed index: %d\n", index);
		return NULL;
	}
	CNP_ITEM *item = CALLOC(1, sizeof(CNP_ITEM));
	if (item)
	{
		char *data = read_data + sizeof(int);
		int data_size = read_size - sizeof(int);
		char *buf = CALLOC(1, data_size);
		if (!buf)
		{
			FREE(item);
			return NULL;
		}
		item->type_index = ((int *)read_data)[0];
		memcpy(buf, data, data_size);
		item->data = buf;
		item->len = data_size;
	}
	return item;
}

static Eina_Bool storage_index_write(StorageData *sd)
{
	CALLED();
	int ret;
	if (!sd->ef)
	{
		DMSG("eet_file is NULL\n");
		return EINA_FALSE;
	}
#ifdef DEBUG
	for (ret = 0; ret < STORAGE_ITEM_CNT; ret++)
		printf(", index %d: %lf", ret, sd->indexTable[ret]);
	printf("\n");
#endif
	ret = eet_write(sd->ef, STORAGE_KEY_INDEX, sd->indexTable, sizeof(indexType) * STORAGE_ITEM_CNT, 1);
	if (ret)
		eet_sync(sd->ef);
	return ret != 0;
}
