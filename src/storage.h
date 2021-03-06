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

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <Eet.h>
#include <Eina.h>
#include <Ecore.h>

#include "item_manager.h"

typedef double indexType; /* Ecore_Time */

#define STORAGE_ITEM_CNT 12
struct _StorageData {
	Eet_File *ef;
	indexType indexTable[STORAGE_ITEM_CNT];
	CNP_ITEM *itemTable[STORAGE_ITEM_CNT];
};

StorageData *init_storage(AppData *ad);
void depose_storage(StorageData *sd);
#endif
