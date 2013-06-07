/*
 * cbhm
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <systemd/sd-daemon.h>
#include <appcore-efl.h>
#include <Ecore_X.h>
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

void *d_malloc(char *func, int line, size_t size)
{
	char *m = malloc(size);
	printf("in %s, %d: 0x%x = malloc(%d)\n", func, line, m, size);
	return m;
}
void *d_calloc(char *func, int line, size_t n, size_t size)
{
	char *m = calloc(n, size);
	printf("in %s, %d: 0x%x = calloc(%d)\n", func, line, m, size);
	return m;
}
void d_free(char *func, int line, void *m)
{
	printf("in %s, %d: free(0x%x)\n", func, line, m);
	free(m);
}

static Eina_Bool setClipboardManager(AppData *ad)
{
	ad->x_disp = ecore_x_display_get();
	DMSG("x_disp: 0x%x\n", ad->x_disp);
	if (ad->x_disp)
	{
		Ecore_X_Atom clipboard_manager_atom = XInternAtom(ad->x_disp, ATOM_CLIPBOARD_MANAGER_NAME, False);
		Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp, clipboard_manager_atom);
		DMSG("clipboard_manager_window: 0x%x\n");
		if (!clipboard_manager)
		{
			ad->x_root_win = DefaultRootWindow(ad->x_disp);
			if (ad->x_root_win)
			{
				ad->x_event_win = ecore_x_window_new(ad->x_root_win, 0, 0, 19, 19);
				DMSG("x_event_win: 0x%x\n", ad->x_event_win);
				if (ad->x_event_win)
				{
					XSetSelectionOwner(ad->x_disp, clipboard_manager_atom, ad->x_event_win, CurrentTime);
					Ecore_X_Window clipboard_manager = XGetSelectionOwner(ad->x_disp, clipboard_manager_atom);
					DMSG("clipboard_manager: 0x%x\n", clipboard_manager);
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
	AppData *ad = data;

	if (!setClipboardManager(ad))
	{
		DMSG("Clipboard Manager set failed\n");
		return EXIT_FAILURE;
	}

	set_x_window(ad->x_event_win, ad->x_root_win);

	if (!ecore_init()) return EXIT_FAILURE;
	if (!ecore_evas_init()) return EXIT_FAILURE;
	if (!edje_init()) return EXIT_FAILURE;
	ad->magic = CBHM_MAGIC;
	init_target_atoms(ad);
	if (!(ad->clipdrawer = init_clipdrawer(ad))) return EXIT_FAILURE;
	if (!(ad->xhandler = init_xhandler(ad))) return EXIT_FAILURE;
	if (!(ad->storage = init_storage(ad))) return EXIT_FAILURE;
	slot_item_count_set(ad);

	set_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD, NULL);
	return 0;
}

static int app_terminate(void *data)
{
	AppData *ad = data;

	depose_clipdrawer(ad->clipdrawer);
	depose_xhandler(ad->xhandler);
	depose_storage(ad->storage);
	item_clear_all(ad);
	depose_target_atoms(ad);
	FREE(ad);

	return 0;
}

static int app_pause(void *data)
{
	AppData *ad = data;
	return 0;
}

static int app_resume(void *data)
{
	AppData *ad = data;
	return 0;
}

static int app_reset(bundle *b, void *data)
{
	AppData *ad = data;

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
	ad = CALLOC(1, sizeof(AppData));
	ops.data = ad;
	g_main_ad = ad;

	appcore_set_i18n(PACKAGE, LOCALEDIR);

	// Notyfication to systemd
	sd_notify(1, "READY=1");

	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}
