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




#ifndef _clipdrawer_h_
#define _clipdrawer_h_

#define CLIPDRAWER_HEIGHT 360
#define CLIPDRAWER_HEIGHT_LANDSCAPE 228

enum {
	GI_TEXT = 0,
	GI_IMAGE,

	GI_MAX_ITEMS,
};

/* view maintains */
int clipdrawer_init(void *data);
int clipdrawer_create_view(void *data);
void clipdrawer_activate_view(void *data);
//void clipdrawer_hide_view(void *data);
void clipdrawer_lower_view(void *data);

void set_rotation_to_clipdrawer(void *data);

const char* clipdrawer_get_plain_string_from_escaped(char *escstr);

char *clipdrawer_get_item_data(void *data, int pos);
int clipdrawer_add_item(char *idata, int type);

void clipdrawer_paste_textonly_set(void *data, Eina_Bool textonly);
Eina_Bool clipdrawer_paste_textonly_get(void *data);

#endif // _clipdrawer_h_
