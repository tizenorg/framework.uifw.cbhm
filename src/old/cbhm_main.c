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


#include "common.h"
#include "cbhm_main.h"
#include "xcnphandler.h"
#include "clipdrawer.h"
#include "scrcapture.h"

#include <vconf.h>
#include <appcore-efl.h>

// FIXME: how to remove g_main_ad? 
static struct appdata *g_main_ad = NULL;
static Evas_Object* create_win(void *data, const char *name);
static Evas_Object* load_edj(Evas_Object *parent, const char *file, const char *group);

static void win_del_cb(void *data, Evas_Object *obj, void *event)
{
	struct appdata *ad = (struct appdata *) data;
	clipdrawer_lower_view(ad);
//  NOTE : cbhm doesn't want to exit by home key.
//	elm_exit();
}

static void main_quit_cb(void *data, Evas_Object* obj, void* event_info)
{
	elm_exit();
}

static int lang_changed(void *data)
{
	return 0;
}

void* g_get_main_appdata()
{
	return (void*)g_main_ad;
}

int init_appview(void *data)
{
	struct appdata *ad = (struct appdata *) data;
	Evas_Object *win, *ly;

	win = create_win(ad, APPNAME);
	if(win == NULL)
		return -1;
	ad->evas = evas_object_evas_get(win);
	ad->win_main = win;
	set_focus_for_app_window(win, EINA_FALSE);

	ly = load_edj(win, APP_EDJ_FILE, GRP_MAIN);
	if (ly == NULL)
		return -1; 
	ad->ly_main = ly;

	evas_object_show(ly);

	clipdrawer_create_view(ad);

	evas_object_smart_callback_add(ad->win_main, "delete,request", win_del_cb, ad);
	edje_object_signal_callback_add(elm_layout_edje_get(ly), "EXIT", "*", main_quit_cb, NULL);

// NOTE: do not show before win_main window resizing
//	evas_object_show(win);

	return 0;
}

void set_focus_for_app_window(Evas_Object *win, Eina_Bool enable)
{
	Eina_Bool accepts_focus;
	Ecore_X_Window_State_Hint initial_state;
	Ecore_X_Pixmap icon_pixmap;
	Ecore_X_Pixmap icon_mask;
	Ecore_X_Window icon_window;
	Ecore_X_Window window_group;
	Eina_Bool is_urgent;

	ecore_x_icccm_hints_get (elm_win_xwindow_get (win),
							 &accepts_focus, &initial_state, &icon_pixmap, &icon_mask, &icon_window, &window_group, &is_urgent);
	ecore_x_icccm_hints_set (elm_win_xwindow_get (win),
							 enable, initial_state, icon_pixmap, icon_mask, icon_window, window_group, is_urgent);
	DTRACE("set focus mode = %d\n", enable);
	ecore_x_flush();
}

static Evas_Object* create_win(void *data, const char *name)
{
	struct appdata *ad = (struct appdata *) data;
	Evas_Object *eo;
	int w, h;
	Ecore_X_Display *dpy;
	dpy = ecore_x_display_get();

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (eo)
	{
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
		ad->root_w = w;	ad->root_h = h;
		evas_object_resize(eo, w, h);
		if (dpy)
		{   //disable window effect
			utilx_set_window_effect_state(dpy, elm_win_xwindow_get(eo), 0);
			ecore_x_icccm_name_class_set(elm_win_xwindow_get(eo), "NORMAL_WINDOW", "NORMAL_WINDOW");
		}
		elm_scale_set((double) w/DEFAULT_WIDTH);
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
	init_scrcapture(ad);

	return 0;
}

static void fini(struct appdata *ad)
{
	close_scrcapture(ad);

	if (ad->ly_main)
		evas_object_del(ad->ly_main);
}

static void init_ad(struct appdata *ad)
{
	g_main_ad = ad;
}

static int app_create(void *data)
{
	struct appdata *ad = data;

	init_ad(ad);

	init(ad);

	lang_changed(ad);

	/* add system event callback */
	appcore_set_event_callback(APPCORE_EVENT_LANG_CHANGE,
			lang_changed, ad);

	return 0;
}


static int app_terminate(void *data)
{
	struct appdata *ad = data;

	fini(ad);

	if (ad->win_main)
		evas_object_del(ad->win_main);

	return 0;
}

static int app_pause(void *data)
{
	return 0;
}

static int app_resume(void *data)
{
	return 0;
}

static int app_reset(bundle *b, void *data)
{
	struct appdata *ad = data;

	if (ad->win_main)
		elm_win_activate(ad->win_main);

	return 0;
}

/*
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
*/

int main(int argc, char *argv[])
{
	struct appdata ad;
	struct appcore_ops ops = {
		.create = app_create,
		.terminate = app_terminate,
		.pause = app_pause,
		.resume = app_resume,
		.reset = app_reset,
	};

	memset(&ad, 0x0, sizeof(struct appdata));
	ops.data = &ad;

	appcore_set_i18n(PACKAGE, LOCALEDIR);

	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
