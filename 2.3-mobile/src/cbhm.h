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

#ifndef _CBHM_H_
#define _CBHM_H_

#include <Elementary.h>
#include <Ecore_X.h>

#if !defined(PACKAGE)
#  define PACKAGE "CBHM"
#endif

#if !defined(APPNAME)
#  define APPNAME "Clipboard History Manager"
#endif

#define CBHM_MAGIC 0xad960009

typedef struct _TargetHandler TargetHandler;
typedef struct _AppData AppData;
typedef struct _ClipdrawerData ClipdrawerData;
typedef struct _CNP_ITEM CNP_ITEM;
typedef struct _XHandlerData XHandlerData;
typedef struct _StorageData StorageData;
typedef char *(*text_converter_func)(AppData *ad, int type_index, const char *str);

#define ITEM_CNT_MAX 20
#define COPIED_DATA_STORAGE_DIR DATADIR"/.cbhm_files"

#include "clipdrawer.h"
#include "item_manager.h"
#include "xhandler.h"
#include "xconverter.h"
#include "storage.h"

struct _TargetHandler {
	Ecore_X_Atom *atom;
	char **name;
	int atom_cnt;
	text_converter_func convert_to_entry;
	text_converter_func convert_to_target[ATOM_INDEX_MAX];
};

struct _AppData {
	int magic;
	Ecore_X_Display *x_disp;
	Ecore_X_Window x_root_win;
	Ecore_X_Window x_event_win;
	Ecore_X_Window x_active_win;
	Eina_List *item_list;

	Eina_Bool (*draw_item_add)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*draw_item_del)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*storage_item_add)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*storage_item_del)(AppData *ad, CNP_ITEM *item);
	Eina_Bool (*storage_item_update)(AppData *ad, CNP_ITEM *item);
	CNP_ITEM *(*storage_item_load)(StorageData *sd, int index);

	ClipdrawerData *clipdrawer;
	XHandlerData *xhandler;
	StorageData *storage;

	CNP_ITEM *clip_selected_item;
	TargetHandler targetAtoms[ATOM_INDEX_MAX];
};

void *d_malloc(const char *func, int line, size_t size);
void *d_calloc(const char *func, int line, size_t n, size_t size);
void d_free(const char *func, int line, void *m);

extern int _log_domain;
#define CRITICAL(...) EINA_LOG_DOM_CRIT(_log_domain, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(_log_domain, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)
#define CALLED() DBG("called %s, %s", __FILE__, __func__);
#define MALLOC(size) d_malloc(__func__, __LINE__, size)
#define CALLOC(n, size) d_calloc(__func__, __LINE__, n, size)
#define FREE(p) d_free(__func__, __LINE__, p)

// Define memory-safe string functions
#define SAFE_STRCMP(s1, s2) ((s1 && s2) ? strcmp(s1, s2) : (s1 ? 1 : -1))
#define SAFE_STRNCMP(s1, s2, n) ((s1 && s2) ? strncmp(s1, s2, n) : (s1 ? 1 : -1))
#define SAFE_STRDUP(s) (s ? strdup(s) : NULL)
#define SAFE_STRNDUP(s, n) (s ? strndup(s, n) : NULL)
#define SAFE_STRCPY(dest, src) (src ? strcpy(dest, src) : NULL)
#define SAFE_STRNCPY(dest, src, n) (src ? strncpy(dest, src, n) : NULL)
#define SAFE_STRCAT(dest, src) ((dest && src) ? strcat(dest, src) : NULL)
#define SAFE_STRNCAT(dest, src, n) ((dest && src) ? strncat(dest, src, n) : NULL)
#define SAFE_STRCHR(s, c) (s ? strchr(s, c) : NULL)
#define SAFE_STRSTR(haystack, needle) (haystack ? strstr(haystack, needle) : NULL)
#define SAFE_STRLEN(s) (s ? strlen(s) : 0)

#endif // _CBHM_H_
