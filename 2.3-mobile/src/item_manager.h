/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#ifndef _ITEM_MANAGER_H_
#define _ITEM_MANAGER_H_

#include "cbhm.h"
#include <vconf.h>
#include <vconf-internal-keys.h>

#define MULTI_(s) dgettext(PACKAGE, s)
#define S_CLIPBOARD MULTI_("IDS_COM_BODY_CLIPBOARD")
#define S_DELETE MULTI_("IDS_COM_BODY_DELETE")
#define S_DONE MULTI_("IDS_COM_BODY_DONE")
#define S_COPY MULTI_("IDS_COM_POP_COPIED_TO_CLIPBOARD")
#define S_EXIST MULTI_("IDS_IME_POP_ITEM_ALREADY_COPIED_TO_CLIPBOARD")
#define S_DELETE_ALL MULTI_("IDS_COM_BODY_DELETE_ALL")
#define S_DELETE_ALL_Q MULTI_("IDS_IME_POP_ALL_ITEMS_WILL_BE_DELETED_FROM_THE_CLIPBOARD")
#define S_CANCEL MULTI_("IDS_COM_BUTTON_CANCEL")
#define S_CLOSE MULTI_("IDS_COM_BODY_CLOSE")
#define S_CLIPBOARD_OPTION MULTI_("IDS_COM_HEADER_CLIPBOARD_OPTIONS")
#define S_DELETE_Q MULTI_("IDS_IME_HEADER_DELETE_ALL_ITEMS_ABB")
#define S_NO_ITEMS MULTI_("IDS_GALLERY_NPBODY_NO_ITEMS")

enum GRID_ITEM_STYLE {
	GRID_ITEM_STYLE_TEXT = 0,
	GRID_ITEM_STYLE_IMAGE = 1,
	GRID_ITEM_STYLE_COMBINED = 2,
	GRID_ITEM_STYLE_MAX = 3
};

struct _CNP_ITEM {
	int type_index;
	void *data;
	size_t len;
	void *file;
	size_t file_len;
	int gitem_style;
	Eina_Bool img_from_web;
	Eina_Bool img_from_markup;
	Elm_Object_Item *gitem;
	Eina_Bool locked;
	AppData *ad;
};

CNP_ITEM *item_add_by_CNP_ITEM(AppData *ad, CNP_ITEM *item, Eina_Bool storage, Eina_Bool show_msg);
CNP_ITEM *item_add_by_data(AppData *ad, Ecore_X_Atom type, void *data, int len, Eina_Bool show_msg);

CNP_ITEM *item_get_by_index(AppData *ad, int index);
CNP_ITEM *item_get_by_data(AppData *ad, void *data, int len);
CNP_ITEM *item_get_last(AppData *ad);

void item_delete_by_CNP_ITEM(AppData *ad, CNP_ITEM *item);
void item_delete_by_data(AppData *ad, void *data, int len);
void item_delete_by_index(AppData *ad, int index);
void item_clear_all(AppData *ad);
int item_count_get(AppData *ad, int atom_index);

#endif /*_ITEM_MANAGER_H_*/

