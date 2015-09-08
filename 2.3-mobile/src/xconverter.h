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

#ifndef _X_ATOM_H_
#define _X_ATOM_H_

enum ATOM_INDEX {
	ATOM_INDEX_TARGET = 0,
	ATOM_INDEX_TEXT = 1,
	ATOM_INDEX_HTML = 2,
	ATOM_INDEX_EFL = 3,
	ATOM_INDEX_IMAGE = 4,
	ATOM_INDEX_POLARIS = 5,
	ATOM_INDEX_MAX = 6
};

typedef struct {
	char emoticon_name[31 + 1];
	char text[24 + 1];
} ENTRY_EMOTICON_S;

typedef enum {
	ENTRY_EMOTICON_NONE = 0,
	ENTRY_EMOTICON_HAPPY,
	ENTRY_EMOTICON_SORRY,
	ENTRY_EMOTICON_WINK,
	ENTRY_EMOTICON_TONGUE_DANGLING,
	ENTRY_EMOTICON_SUPRISED,
	ENTRY_EMOTICON_KISS,
	ENTRY_EMOTICON_ANGRY_SHOUT,
	ENTRY_EMOTICON_SMILE,
	ENTRY_EMOTICON_OMG,
	ENTRY_EMOTICON_LITTLE_BIT_SORRY,

	ENTRY_EMOTICON_VERY_SORRY = 11,
	ENTRY_EMOTICON_GUILTY,
	ENTRY_EMOTICON_HAHA,
	ENTRY_EMOTICON_WORRIED,
	ENTRY_EMOTICON_LOVE,
	ENTRY_EMOTICON_EVIL,
	ENTRY_EMOTICON_HALF_SMILE,
	ENTRY_EMOTICON_MINIMAL_SMILE,
	ENTRY_EMOTICON_MAX
} ENTRY_EMOTICON_TYPE_E;

#include "cbhm.h"

void init_target_atoms(AppData *ad);
void depose_target_atoms(AppData *ad);
int atom_type_index_get(AppData *ad, Ecore_X_Atom atom);
char *string_for_entry_get(AppData *ad, int type_index, const char *str);
char *string_for_image_path_get(AppData *ad, int type_index, const char *str);
Eina_Bool generic_converter(AppData *ad, Ecore_X_Atom reqAtom, CNP_ITEM *item, void **data_ret, int *size_ret, Ecore_X_Atom *ttype, int *tsize);
char *entry_convert_emoticon_to_normal_text(const char *src_text);

#endif
