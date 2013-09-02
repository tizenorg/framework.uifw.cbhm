/*
 * cbhm
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#include "item_manager.h"

static void item_free(CNP_ITEM *item)
{
	CALLED();
	if (!item)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return;
	}
	// remove gengrid
	if (item->ad)
	{
		if (item->ad->draw_item_del)
			item->ad->draw_item_del(item->ad, item);
		if (item->ad->storage_item_del)
			item->ad->storage_item_del(item->ad, item);
	}
	if (item->data)
	{
		if(item->type_index == ATOM_INDEX_IMAGE)
			ecore_file_remove(item->data);

		FREE(item->data);
	}

	if (item->ad)
	{
		if (item->ad->clip_selected_item == item)
		item->ad->clip_selected_item = NULL;
	}
	FREE(item);
}

CNP_ITEM *item_add_by_CNP_ITEM(AppData *ad, CNP_ITEM *item)
{
	if (!ad || !item)
	{
		DMSG("WRONG PARAMETER in %s, ad: 0x%x, item: 0x%x\n", __func__, ad, item);
		return NULL;
	}
	item->ad = ad;

	ad->item_list = eina_list_prepend(ad->item_list, item);
	if (ad && ad->draw_item_add)
		ad->draw_item_add(ad, item);
	if (ad && ad->storage_item_add)
		ad->storage_item_add(ad, item);

	while (ITEM_CNT_MAX < eina_list_count(ad->item_list))
	{
		CNP_ITEM *ditem = eina_list_nth(ad->item_list, ITEM_CNT_MAX);

		ad->item_list = eina_list_remove(ad->item_list, ditem);
		item_free(ditem);
	}

	slot_property_set(ad, -1);
	slot_item_count_set(ad);

	return item;
}

CNP_ITEM *item_add_by_data(AppData *ad, Ecore_X_Atom type, void *data, int len)
{
	char *copied_path = NULL;
	char *filename = NULL;
	int size_path=0;

	if (!ad || !data)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return NULL;
	}
	CNP_ITEM *item;
	item = CALLOC(1, sizeof(CNP_ITEM));
	if (!item)
		return NULL;
	item->type_index = atom_type_index_get(ad, type);

	if(item->type_index == ATOM_INDEX_IMAGE)
	{
		filename = ecore_file_file_get(data);
		size_path = snprintf(NULL, 0, COPIED_DATA_STORAGE_DIR"/%s", filename) + 1;
		copied_path = MALLOC(sizeof(char) * size_path);

		if(copied_path)
			snprintf(copied_path, size_path, COPIED_DATA_STORAGE_DIR"/%s", filename);

		if(!ecore_file_cp(data, copied_path))
			DMSG("ecore_file_cp fail!");

		data = copied_path;
		len = strlen(copied_path) + 1;
	}

	item->data = data;
	item->len = len;

	item = item_add_by_CNP_ITEM(ad, item);
	return item;
}

CNP_ITEM *item_get_by_index(AppData *ad, int index)
{
	if (!ad || eina_list_count(ad->item_list) <= index || 0 > index)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return NULL;
	}
	CNP_ITEM *item;
	item = eina_list_nth(ad->item_list, index);
	return item;
}

CNP_ITEM *item_get_by_data(AppData *ad, void *data)
{
	if (!ad || !data)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return NULL;
	}
	CNP_ITEM *item;
	Eina_List *l;
	EINA_LIST_FOREACH(ad->item_list, l, item)
	{
		if (item && item->data == data)
		{
			return item;
		}
	}
	return NULL;
}

CNP_ITEM *item_get_last(AppData *ad)
{
	if (!ad)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return NULL;
	}
	return eina_list_data_get(ad->item_list);
}

void item_delete_by_CNP_ITEM(AppData *ad, CNP_ITEM *item)
{
	CALLED();
	if (!ad || !item)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return;
	}
	DMSG("item: 0x%x, item->gitem: 0x%x\n", item, item->gitem);
	ad->item_list = eina_list_remove(ad->item_list, item);
	item_free(item);
	slot_property_set(ad, -1);
	slot_item_count_set(ad);
}

void item_delete_by_data(AppData *ad, void *data)
{
	CALLED();
	if (!ad || !data)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return;
	}
	CNP_ITEM *item;
	item = item_get_by_data(ad, data);
	item_delete_by_CNP_ITEM(ad, item);
}

void item_delete_by_index(AppData *ad, int index)
{
	CALLED();
	if (!ad || eina_list_count(ad->item_list) <= index || 0 > index)
	{
		DMSG("WRONG PARAMETER in %s\n", __func__);
		return;
	}
	CNP_ITEM *item;
	item = item_get_by_index(ad, index);
	item_delete_by_CNP_ITEM(ad, item);
}

void item_clear_all(AppData *ad)
{
	CALLED();
	while(ad->item_list)
	{
		CNP_ITEM *item = eina_list_data_get(ad->item_list);
		ad->item_list = eina_list_remove(ad->item_list, item);
		if (item)
			item_free(item);
	}
}

int item_count_get(AppData *ad)
{
	return eina_list_count(ad->item_list);
}
