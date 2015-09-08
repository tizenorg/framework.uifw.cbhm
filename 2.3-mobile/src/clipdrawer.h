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

#ifndef _CLIPDRAWER_H_
#define _CLIPDRAWER_H_

#include <Ecore_X.h>
#include <Elementary.h>
typedef enum _AnimStatus AnimStatus;
enum _AnimStatus {
	STATUS_NONE = 0,
	SHOW_ANIM,
	HIDE_ANIM
};

struct _ClipdrawerData {
	Evas_Object *main_win;
	Evas_Object *gengrid;
	Evas_Object *main_layout;
	Elm_Gengrid_Item_Class gic_text;
	Elm_Gengrid_Item_Class gic_image;
	Elm_Gengrid_Item_Class gic_combined;
	Evas_Object *popup;
	Evas_Object *popup_win;
	Evas_Object *popup_conform;
	Evas_Object *cbhm_popup;
	Evas *evas;
	Ecore_Event_Handler *keydown_handler;
	Ecore_Timer *anim_timer;
	Elm_Gengrid_Item_Class gic;
	Ecore_Timer *lower_view_timer;
	int locked_item_count;
	Ecore_X_Window x_main_win;

	int o_degree;

	int root_w;
	int root_h;

	int height;
	int landscape_height;
	int grid_item_bg_w;
	int grid_item_bg_h;
	int grid_image_item_w;
	int grid_image_item_h;

	AnimStatus anim_status;
	int anim_count;
	Eina_Bool popup_activate:1;
	Eina_Bool paste_text_only:1;
	Eina_Bool item_clicked:1;
	Eina_Bool delbtn_clicked:1;
	Eina_Bool http_path:1;
	Evas_Object *event_rect;
	Evas_Object *gesture_layer;
};

#include "cbhm.h"

Eina_Bool delete_mode;

void set_rotation_to_clipdrawer(AppData *ad);
void clipdrawer_activate_view(AppData* ad);
void clipdrawer_lower_view(AppData* ad);
ClipdrawerData *init_clipdrawer(AppData *ad);
void depose_clipdrawer(ClipdrawerData *cd);
void _delete_mode_set(AppData *ad, Eina_Bool del_mode);
void clipdrawer_paste_textonly_set(AppData *ad, Eina_Bool textonly);

#endif // _CLIPDRAWER_H_
