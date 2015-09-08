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

#ifndef _STORAGE_H_
#define _STORAGE_H_

#include <Eet.h>
#include <Eina.h>
#include <Ecore.h>

#include "cbhm.h"

typedef double indexType; /* Ecore_Time */

struct _StorageData {
	Eet_File *ef;
	indexType indexTable[ITEM_CNT_MAX];
	CNP_ITEM *itemTable[ITEM_CNT_MAX];
};

StorageData *init_storage(AppData *ad);
void depose_storage(StorageData *sd);
#endif
