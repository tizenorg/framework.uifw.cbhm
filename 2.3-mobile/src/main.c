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
#include <appcore-efl.h>
#include <Ecore_X.h>
#include <systemd/sd-daemon.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>

#include "cbhm.h"

#define CLIPBOARD_MANAGER_WINDOW_TITLE_STRING "X11_CLIPBOARD_HISTORY_MANAGER"
#define ATOM_CLIPBOARD_MANAGER_NAME "CLIPBOARD_MANAGER"

static AppData *g_main_ad = NULL;

int _log_domain = -1;

void *d_malloc(const char *func, int line, size_t size)
{
	char *m = malloc(size);
	DBG("in %s, %d: 0x%p = malloc(%d)", func, line, m, size);
	return m;
}
void *d_calloc(const char *func, int line, size_t n, size_t size)
{
	char *m = calloc(n, size);
	DBG("in %s, %d: 0x%p = calloc(%d)", func, line, m, size);
	return m;
}
void d_free(const char *func, int line, void *m)
{
	DBG("in %s, %d: free(0x%p)", func, line, m);
	free(m);
}

static Eina_Bool setClipboardManager(AppData *ad)
{
	if (!ad) return EINA_FALSE;

	if(!ecore_file_mkpath(COPIED_DATA_STORAGE_DIR))
		DBG("ecore_file_mkpath fail");

	ad->x_disp = ecore_x_display_get();
	DBG("x_disp: 0x%p", ad->x_disp);
	if (ad->x_disp)
	{
		Ecore_X_Atom clipboard_manager_atom = XInternAtom(ad->x_disp, ATOM_CLIPBOARD_MANAGER_NAME, False);
		Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp, clipboard_manager_atom);
		DBG("clipboard_manager_window: 0x%x", clipboard_manager);
		if (!clipboard_manager)
		{
			ad->x_root_win = DefaultRootWindow(ad->x_disp);
			if (ad->x_root_win)
			{
				ad->x_event_win = ecore_x_window_new(ad->x_root_win, 0, 0, 19, 19);
				DBG("x_event_win: 0x%x", ad->x_event_win);
				if (ad->x_event_win)
				{
					XSetSelectionOwner(ad->x_disp, clipboard_manager_atom, ad->x_event_win, CurrentTime);
					Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp, clipboard_manager_atom);
					DBG("clipboard_manager: 0x%x", clipboard_manager);
					if (ad->x_event_win == clipboard_manager)
					{
						return EINA_TRUE;
					}
				}
			}
		}
	}
	return EINA_FALSE;
}

static void set_x_window(Ecore_X_Window x_event_win, Ecore_X_Window x_root_win)
{
	ecore_x_netwm_name_set(x_event_win, CLIPBOARD_MANAGER_WINDOW_TITLE_STRING);
	ecore_x_event_mask_set(x_event_win,
			ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
	ecore_x_event_mask_set(x_root_win,
			ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
	ecore_x_window_prop_property_set(
			x_root_win, ecore_x_atom_get("CBHM_XWIN"),
			XA_WINDOW, 32, &x_event_win, 1);
	ecore_x_flush();
}

static int app_create(void *data)
{
	AppData *ad = (AppData *)data;

   elm_app_base_scale_set(1.8);
	ecore_x_init(ad->x_disp);
	_log_domain = eina_log_domain_register("cbhm", EINA_COLOR_LIGHTBLUE);
	if (!_log_domain)
		{
			EINA_LOG_ERR("could not register cbhm log domain.");
			_log_domain = EINA_LOG_DOMAIN_GLOBAL;
		}

	if (!setClipboardManager(ad))
	{
		DBG("Clipboard Manager set failed");
		return EXIT_FAILURE;
	}

	set_x_window(ad->x_event_win, ad->x_root_win);

	if (!ecore_init()) return EXIT_FAILURE;
	if (!ecore_evas_init()) return EXIT_FAILURE;
	if (!edje_init()) return EXIT_FAILURE;
	ad->magic = CBHM_MAGIC;
	init_target_atoms(ad);
	if (!(ad->clipdrawer = init_clipdrawer(ad))) return EXIT_FAILURE;

	//set env for root.
	setenv("HOME", "/", 1);
	if (!(ad->xhandler = init_xhandler(ad))) return EXIT_FAILURE;
	if (!(ad->storage = init_storage(ad))) return EXIT_FAILURE;
	slot_item_count_set(ad);

	set_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD, NULL);
	return 0;
}

static int app_terminate(void *data)
{
	AppData *ad = data;

	item_clear_all(ad);
	depose_clipdrawer(ad->clipdrawer);
	depose_xhandler(ad->xhandler);
	depose_storage(ad->storage);
	depose_target_atoms(ad);
	FREE(ad);

	eina_log_domain_unregister(_log_domain);
	_log_domain = -1;

	return 0;
}

static int app_pause(void *data)
{
	AppData *ad = data;
	Ecore_X_Illume_Clipboard_State state = ecore_x_e_illume_clipboard_state_get(ad->x_active_win);
	if(state == ECORE_X_ILLUME_CLIPBOARD_STATE_ON)
	{
		clipdrawer_lower_view(ad);
	}
	return 0;
}

static int app_resume(void *data)
{
	return 0;
}

static int app_reset(bundle *b, void *data)
{
	return 0;
}

int main(int argc, char *argv[])
{
	AppData *ad;

	struct appcore_ops ops = {
		.create = app_create,
		.terminate = app_terminate,
		.pause = app_pause,
		.resume = app_resume,
		.reset = app_reset,
	};

	ad = calloc(1, sizeof(AppData));
	ops.data = ad;
	g_main_ad = ad;

	appcore_set_i18n(PACKAGE, LOCALEDIR);

	// Notyfication to systemd
	sd_notify(1, "READY=1");

	//set env app for changeable UI.
	setenv("HOME", "/opt/home/app" , 1);

	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
