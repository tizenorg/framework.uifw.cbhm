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
	Ecore_X_Window x_main_win;
	Evas_Object *gengrid;
	Evas_Object *main_layout;
	Elm_Gengrid_Item_Class gic;
	Evas_Object *popup;

	int o_degree;

	int root_w;
	int root_h;

	int height;
	int landscape_height;
	int grid_item_w;
	int grid_item_h;

	Ecore_Event_Handler *keydown_handler;
	Evas *evas;

	Ecore_Timer *anim_timer;
	AnimStatus anim_status;
	int anim_count;
	Eina_Bool paste_text_only:1;
};

#include "cbhm.h"

Eina_Bool delete_mode;

void set_rotation_to_clipdrawer(ClipdrawerData *ad);
void clipdrawer_activate_view(AppData* ad);
void clipdrawer_lower_view(AppData* ad);
ClipdrawerData *init_clipdrawer(AppData *ad);
void depose_clipdrawer(ClipdrawerData *cd);
void _delete_mode_set(AppData *ad, Eina_Bool del_mode);

#endif // _CLIPDRAWER_H_
