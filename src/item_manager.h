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




#ifndef _ITEM_MANAGER_H_
#define _ITEM_MANAGER_H_

#include "cbhm.h"

struct _CNP_ITEM {
	int type_index;
	void *data;
	size_t len;

	Elm_Object_Item *gitem;
	Evas_Object *layout;
	AppData *ad;
};

CNP_ITEM *item_add_by_CNP_ITEM(AppData *ad, CNP_ITEM *item);
CNP_ITEM *item_add_by_data(AppData *ad, Ecore_X_Atom type, void *data, int len);

CNP_ITEM *item_get_by_index(AppData *ad, int index);
CNP_ITEM *item_get_by_data(AppData *ad, void *data);
CNP_ITEM *item_get_last(AppData *ad);

void item_delete_by_CNP_ITEM(AppData *ad, CNP_ITEM *item);
void item_delete_by_data(AppData *ad, void *data);
void item_delete_by_index(AppData *ad, int index);
void item_clear_all(AppData *ad);
int item_count_get(AppData *ad);

#endif /*_ITEM_MANAGER_H_*/

