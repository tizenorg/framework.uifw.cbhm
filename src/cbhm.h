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

#if !defined(LOCALEDIR)
#  define LOCALEDIR "/usr/share/locale"
#endif

#define CBHM_MAGIC 0xad960009

typedef struct _TargetHandler TargetHandler;
typedef struct _AppData AppData;
typedef struct _ClipdrawerData ClipdrawerData;
typedef struct _CNP_ITEM CNP_ITEM;
typedef struct _XHandlerData XHandlerData;
typedef struct _SCaptureData SCaptureData;
typedef struct _StorageData StorageData;
typedef char *(*text_converter_func)(AppData *ad, int type_index, const char *str);

#include "clipdrawer.h"
#include "item_manager.h"
#include "xhandler.h"
#include "xconverter.h"
#include "scrcapture.h"
#include "storage.h"

struct _TargetHandler {
	Ecore_X_Atom *atom;
	char **name;
	int atom_cnt;
	text_converter_func convert_for_entry;
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
//	CNP_ITEM *(*storage_item_load)(AppData *ad, int index);

	ClipdrawerData *clipdrawer;
	XHandlerData *xhandler;
	SCaptureData *screencapture;
	StorageData *storage;

	CNP_ITEM *clip_selected_item;
	TargetHandler targetAtoms[ATOM_INDEX_MAX];
};

void *d_malloc(char *func, int line, size_t size);
void *d_calloc(char *func, int line, size_t n, size_t size);
void d_free(char *func, int line, void *m);

//#define DEBUG

#ifdef DEBUG
#define DTRACE(fmt, args...) \
{do { fprintf(stderr, "[%s:%04d] " fmt, __func__,__LINE__, ##args); } while (0); }
#define DMSG(fmt, args...) printf("[%s] " fmt, __func__, ## args )
#define CALLED() printf("called %s, %s\n", __FILE__, __func__);
#define DTIME(fmt, args...) \
{do { struct timeval tv1; gettimeofday(&tv1, NULL); double t1=tv1.tv_sec+(tv1.tv_usec/1000000.0); fprintf(stderr, "[CBHM][time=%lf:%s:%04d] " fmt, t1, __func__, __LINE__, ##args); } while (0); }
#ifdef MEM_DEBUG
#define MALLOC(size) d_malloc(__func__, __LINE__, size)
#define CALLOC(n, size) d_calloc(__func__, __LINE__, n, size)
#define FREE(p) d_free(__func__, __LINE__, p)
#else
#define MALLOC(size) malloc(size)
#define CALLOC(n, size) calloc(n, size)
#define FREE(p) free(p)
#endif
#else
#define DTRACE(fmt, args...)
#define DMSG(fmt, args...)
#define CALLED()
#define DTIME(fmt, args...)
#define MALLOC(size) malloc(size)
#define CALLOC(n, size) calloc(n, size)
#define FREE(p) free(p)
#endif

#endif // _CBHM_H_
