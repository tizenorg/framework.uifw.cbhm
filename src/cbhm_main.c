/*
 * SLP2
 * Copyright (c) 2009 Samsung Electronics, Inc.
 * All rights reserved.
 *
 * This software is a confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung Electronics.
 */

#include <stdio.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <utilX.h>
//#include <appcore-efl.h>

#include "common.h"
#include "cbhm_main.h"
#include "xcnphandler.h"
#include "clipdrawer.h"
#include "scrcapture.h"

static Evas_Object* create_win(const char *name);
static Evas_Object* load_edj(Evas_Object *parent, const char *file, const char *group);

static void win_del_cb(void *data, Evas_Object *obj, void *event)
{
	elm_exit();
}

static void main_quit_cb(void *data, Evas_Object* obj, void* event_info)
{
	elm_exit();
}

int init_appview(void *data)
{
	struct appdata *ad = (struct appdata *) data;
	Evas_Object *win, *ly;

	win = create_win(APPNAME);
	if(win == NULL)
		return -1;
	ad->evas = evas_object_evas_get(win);
	ad->win_main = win;

	ly = load_edj(win, APP_EDJ_FILE, GRP_MAIN);
	if (ly == NULL)
		return -1; 
	elm_win_resize_object_add(win, ly);
	edje_object_signal_callback_add(elm_layout_edje_get(ly), "EXIT", "*", main_quit_cb, NULL);
	ad->ly_main = ly;

	evas_object_show(ly);

	clipdrawer_create_view(ad);

//	evas_object_show(win);

	return 0;
}

static Evas_Object* create_win(const char *name)
{
	Evas_Object *eo;
	int w, h;

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (eo)
    {
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		evas_object_smart_callback_add(eo, "delete,request", win_del_cb, NULL);
		ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
		evas_object_resize(eo, w, h);
	}

	return eo;
}

static Evas_Object* load_edj(Evas_Object *parent, const char *file, const char *group)
{       
	Evas_Object *eo;
	int ret;

	eo = elm_layout_add(parent);
	if (eo) 
	{
		ret = elm_layout_file_set(eo, file, group);
		if (!ret) 
		{
			evas_object_del(eo);
			return NULL;
		}

		evas_object_size_hint_weight_set(eo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_win_resize_object_add(parent, eo);
	}

	return eo;
}

static int init(struct appdata *ad)
{
	/* FIXME : add checking multiple instance */

	xcnp_init(ad);
	init_appview(ad);
//	init_scrcapture(ad);

	return 0;
}

static void fini(struct appdata *ad)
{
	if (ad->ly_main)
		evas_object_del(ad->ly_main);

	if (ad->win_main)
		evas_object_del(ad->win_main);
}

static void init_ad(struct appdata *ad)
{
	memset(ad, 0x0, sizeof(struct appdata));
}

EAPI int elm_main(int argc, char **argv)
{
	struct appdata ad;

	init_ad(&ad);

	if(!init(&ad))
		elm_run();

	fini(&ad);

	elm_shutdown();

	return EXIT_SUCCESS;
}

int main( int argc, char *argv[] )
{
//	setenv("ELM_THEME", "beat", 1); // not recommended way

	elm_init(argc, argv);

	return elm_main(argc, argv);
}
