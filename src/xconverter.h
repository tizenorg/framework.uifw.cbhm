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




#ifndef _X_ATOM_H_
#define _X_ATOM_H_

enum ATOM_INDEX {
	ATOM_INDEX_TARGET = 0,
	ATOM_INDEX_TEXT = 1,
	ATOM_INDEX_HTML = 2,
	ATOM_INDEX_EFL = 3,
	ATOM_INDEX_IMAGE = 4,
	ATOM_INDEX_MAX = 5
};

#include "cbhm.h"

void init_target_atoms(AppData *ad);
void depose_target_atoms(AppData *ad);
int atom_type_index_get(AppData *ad, Ecore_X_Atom atom);
char *string_for_entry_get(AppData *ad, int type_index, const char *str);
Eina_Bool generic_converter(AppData *ad, Ecore_X_Atom reqAtom, CNP_ITEM *item, void **data_ret, int *size_ret, Ecore_X_Atom *ttype, int *tsize);

#endif
