/*
 * Copyright (c) 2009 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information"). You agree and
 * acknowledge that this software is owned by Samsung and you shall
 * not disclose such Confidential Information and shall use it only
 * in accordance with the terms of the license agreement you entered
 * into with SAMSUNG ELECTRONICS.  SAMSUNG make no representations
 * or warranties about the suitability of the software, either express
 * or implied, including but not limited to the implied warranties of
 * merchantability, fitness for a particular purpose, or non-infringement.
 * SAMSUNG shall not be liable for any damages suffered by
 * licensee arising out of or releated to this software.
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
