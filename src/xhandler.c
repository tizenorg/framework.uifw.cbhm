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

#include "xhandler.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>

Ecore_X_Window get_selection_owner(AppData *ad, Ecore_X_Selection selection)
{
	CALLED();
	if (!ad) return 0;
	Ecore_X_Atom sel = 0;
	switch(selection)
	{
		case ECORE_X_SELECTION_SECONDARY:
			sel = ECORE_X_ATOM_SELECTION_SECONDARY;
			break;
		case ECORE_X_SELECTION_CLIPBOARD:
			sel = ECORE_X_ATOM_SELECTION_CLIPBOARD;
			break;
		default:
			return 0;
	}
	return XGetSelectionOwner(ad->x_disp, sel);
}

Eina_Bool is_cbhm_selection_owner(AppData *ad, Ecore_X_Selection selection)
{
	CALLED();
	if (!ad) return EINA_FALSE;
	Ecore_X_Window sel_owner = get_selection_owner(ad, selection);
	DMSG("selection_owner: 0x%x, x_event_win: 0x%x \n", sel_owner, ad->x_event_win);
	if (sel_owner == ad->x_event_win)
		return EINA_TRUE;
	return EINA_FALSE;
}

Eina_Bool set_selection_owner(AppData *ad, Ecore_X_Selection selection, CNP_ITEM *item)
{
	CALLED();
	if (!ad) return EINA_FALSE;

	if (!item && is_cbhm_selection_owner(ad, selection))
		return EINA_TRUE;

	Ecore_X_Atom sel = 0;
	Eina_Bool (*selection_func)(Ecore_X_Window win, const void *data, int size) = NULL;

	switch(selection)
	{
		case ECORE_X_SELECTION_SECONDARY:
//			ecore_x_selection_secondary_clear();
			selection_func = ecore_x_selection_secondary_set;
			ad->clip_selected_item = item;
			break;
		case ECORE_X_SELECTION_CLIPBOARD:
//			ecore_x_selection_clipboard_clear();
			selection_func = ecore_x_selection_clipboard_set;
			break;
		default:
			return EINA_FALSE;
	}

	if (selection_func(ad->x_event_win, NULL, 0))
		return EINA_TRUE;

	DMSG("ERROR: set selection failed\n");
	return EINA_FALSE;
}

static Eina_Bool selection_timer_cb(void *data)
{
	CALLED();
	AppData *ad = data;
	XHandlerData *xd = ad->xhandler;

	set_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD, NULL);
	if (is_cbhm_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD))
	{
		ecore_timer_del(xd->selection_timer);
		xd->selection_timer = NULL;
		return ECORE_CALLBACK_CANCEL;
	}
	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _xsel_clear_cb(void *data, int type, void *event)
{
	CALLED();
	if (!data || !event) return EINA_TRUE;
	AppData *ad = data;
	XHandlerData *xd = ad->xhandler;
	Ecore_X_Event_Selection_Clear *ev = event;

	DMSG("in %s, ev->win: 0x%x\n", __func__, ev->win);

	if (is_cbhm_selection_owner(ad, ev->selection)) return EINA_TRUE;
	if (ev->selection != ECORE_X_SELECTION_CLIPBOARD)
		return ECORE_CALLBACK_PASS_ON;

	ecore_x_selection_clipboard_request(ad->x_event_win, ECORE_X_SELECTION_TARGET_TARGETS);

	if (xd->selection_timer)
	{
		ecore_timer_del(xd->selection_timer);
		xd->selection_timer = NULL;
	}
	xd->selection_timer = ecore_timer_add(SELECTION_CHECK_TIME, selection_timer_cb, ad);

	return ECORE_CALLBACK_DONE;
}

static Eina_Bool _xsel_request_cb(void *data, int type, void *event)
{
	CALLED();
	if (!data || !event) return ECORE_CALLBACK_PASS_ON;
	AppData *ad = data;
	Ecore_X_Event_Selection_Request *ev = event;

#ifdef DEBUG
	char *names[3];
	DMSG("selection_owner: 0x%x, ev->... owner: 0x%x, req: 0x%x, selection: %s, target: %s, property: %s\n",
			get_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD), ev->owner, ev->requestor,
			names[0] = ecore_x_atom_name_get(ev->selection),
			names[1] = ecore_x_atom_name_get(ev->target),
			names[2] = ecore_x_atom_name_get(ev->property));
	FREE(names[0]);
	FREE(names[1]);
	FREE(names[2]);
#endif

	CNP_ITEM *item = NULL;
	if (ev->selection == ECORE_X_ATOM_SELECTION_CLIPBOARD)
		item = item_get_last(ad);
	else if (ev->selection == ECORE_X_ATOM_SELECTION_SECONDARY)
		item = ad->clip_selected_item;
	else
		return ECORE_CALLBACK_PASS_ON;

	if (!item)
	{
		DMSG("has no item\n");
		ecore_x_selection_notify_send(ev->requestor,
				ev->selection,
				None,
				None,
				CurrentTime);
		DMSG("change property notify\n");
		ecore_x_flush();
		return ECORE_CALLBACK_DONE;
	}

	Ecore_X_Atom property = None;
	void *data_ret = NULL;
	int size_ret;
	Ecore_X_Atom ttype;
	int tsize;

	if (!generic_converter(ad, ev->target, item, &data_ret, &size_ret, &ttype, &tsize))
		/*	if (!ecore_x_selection_convert(ev->selection,
			ev->target,
			&data_ret, &len, &typeAtom, &typesize))*/

	{
		/* Refuse selection, conversion to requested target failed */
		DMSG("converter return FALSE\n");
	}
	else if (data_ret)
	{
		/* FIXME: This does not properly handle large data transfers */
		ecore_x_window_prop_property_set(
				ev->requestor,
				ev->property,
				ttype,
				tsize,
				data_ret,
				size_ret);
		property = ev->property;
		FREE(data_ret);
		DMSG("change property\n");
	}

	ecore_x_selection_notify_send(ev->requestor,
			ev->selection,
			ev->target,
			property,
			CurrentTime);
	DMSG("change property notify\n");
	ecore_x_flush();
	return ECORE_CALLBACK_DONE;
}

static void send_convert_selection_target(AppData *ad, Ecore_X_Selection_Data_Targets *targets_data)
{
	CALLED();
	/*	struct _Ecore_X_Selection_Data_Targets {
		Ecore_X_Selection_Data data;
		struct _Ecore_X_Selection_Data {
		enum {
		ECORE_X_SELECTION_CONTENT_NONE,
		ECORE_X_SELECTION_CONTENT_TEXT,
		ECORE_X_SELECTION_CONTENT_FILES,
		ECORE_X_SELECTION_CONTENT_TARGETS,
		ECORE_X_SELECTION_CONTENT_CUSTOM
		} content;
		unsigned char *data;
		int            length;
		int            format;
		int            (*FREE)(void *data);
		};

		char                 **targets;
		int                    num_targets;
		};*/
	if (!targets_data || !ad)
		return;
	Ecore_X_Atom *atomlist = (Ecore_X_Atom *)targets_data->data.data;
	if (!atomlist)
		return;

	DMSG("targets_data->num_targets: 0x%x\n", targets_data->num_targets);
	int i, j, k;
	for (i = 0; i < targets_data->num_targets; i++)
	{
		DMSG("get target: %s\n", targets_data->targets[i]);
		for (j = 0; j < ATOM_INDEX_MAX; j++)
		{
			for (k = 0; k < ad->targetAtoms[j].atom_cnt; k++)
			{
				if (!strcmp(targets_data->targets[i], ad->targetAtoms[j].name[k]))
				{
					DMSG("find matched target: %s\n", ad->targetAtoms[j].name[k]);
					ecore_x_selection_clipboard_request(ad->x_event_win, ad->targetAtoms[j].name[k]);
					return;
				}
			}
		}
	}
	DMSG("ERROR: get target atom failed\n");
}

static Eina_Bool _add_selection_imagepath(AppData* ad, char *str)
{
	if (!ad || !str)
		return EINA_FALSE;
	DMSG("get FILE: %s\n", str);
	char *slash = strchr(str, '/');
	while (slash && slash[0] == '/')
	{
		if (slash[1] != '/')
		{
			char *filepath;
			filepath = strdup(slash);
			if (filepath)
			{
				if (ecore_file_exists(filepath))
				{
					item_add_by_data(ad, ad->targetAtoms[ATOM_INDEX_IMAGE].atom[0], filepath, strlen(filepath) + 1);
					return EINA_TRUE;
				}
				else
					FREE(filepath);
			}
			break;
		}
		slash++;
	}
	DMSG("Error : it isn't normal file = %s\n", str);
	return EINA_FALSE;
}

static void _get_selection_data_files(AppData* ad, Ecore_X_Selection_Data_Files *files_data)
{
/*	struct _Ecore_X_Selection_Data_Files {
		Ecore_X_Selection_Data data;
		char                 **files;
		int                    num_files;
	}; */

	int i;
	for (i = 0; i < files_data->num_files; i++)
	{
		_add_selection_imagepath(ad, files_data->files[i]);
	}
}

static Eina_Bool _xsel_notify_cb(void *data, int type, void *event)
{
	CALLED();
	if (!data || !event)
		return ECORE_CALLBACK_PASS_ON;

	AppData *ad = data;
	XHandlerData *xd = ad->xhandler;
	if (xd->selection_timer)
	{
		ecore_timer_del(xd->selection_timer);
		xd->selection_timer = NULL;
	}

/*	struct _Ecore_X_Event_Selection_Notify
	{
		Ecore_X_Window    win;
		Ecore_X_Time      time;
		Ecore_X_Selection selection;
		Ecore_X_Atom      atom;
		char             *target;
		void             *data;
	};*/
	Ecore_X_Event_Selection_Notify *ev = event;

	switch (ev->selection)
	{
		case ECORE_X_SELECTION_CLIPBOARD:
			break;
		case ECORE_X_SELECTION_SECONDARY:
		case ECORE_X_SELECTION_PRIMARY:
		case ECORE_X_SELECTION_XDND:
		default:
			return ECORE_CALLBACK_PASS_ON;
	}
	if (!ev->data)
		goto set_clipboard_selection_owner;

/*	struct _Ecore_X_Selection_Data {
		enum {
			ECORE_X_SELECTION_CONTENT_NONE,
			ECORE_X_SELECTION_CONTENT_TEXT,
			ECORE_X_SELECTION_CONTENT_FILES,
			ECORE_X_SELECTION_CONTENT_TARGETS,
			ECORE_X_SELECTION_CONTENT_CUSTOM
		} content;
		unsigned char *data;
		int            length;
		int            format;
		int            (*FREE)(void *data);
	};*/
	Ecore_X_Selection_Data *sel_data = ev->data;
	switch (sel_data->content)
	{
		case ECORE_X_SELECTION_CONTENT_NONE:
			DMSG("ECORE_X_SELECTION_CONTENT_NONE\n");
			break;
		case ECORE_X_SELECTION_CONTENT_TEXT:
			DMSG("ECORE_X_SELECTION_CONTENT_TEXT\n");
		/*	struct _Ecore_X_Selection_Data_Text {
				Ecore_X_Selection_Data data;
				char                  *text;
			};
			Ecore_X_Selection_Data_Text *text_data = ev->data;*/
		//	DMSG("sel_data->data: 0x%x, text_data->text: 0x%x\n", sel_data->data, text_data->text);
			break;
		case ECORE_X_SELECTION_CONTENT_FILES:
			DMSG("ECORE_X_SELECTION_CONTENT_FILES\n");
			_get_selection_data_files(ad, ev->data);
			goto set_clipboard_selection_owner;
			break;
		case ECORE_X_SELECTION_CONTENT_TARGETS:
			DMSG("ECORE_X_SELECTION_CONTENT_TARGETS\n");
			send_convert_selection_target(ad, ev->data);
			if (!is_cbhm_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD))
				xd->selection_timer = ecore_timer_add(SELECTION_CHECK_TIME, selection_timer_cb, ad);
			return ECORE_CALLBACK_DONE;
		case ECORE_X_SELECTION_CONTENT_CUSTOM:
			DMSG("ECORE_X_SELECTION_CONTENT_CUSTOM\n");
			break;
	}
#ifdef DEBUG
	char *name;
	DMSG("get atom: %d(%s), target: %s, length: %d, format: %d\n",
			ev->atom, name = ecore_x_atom_name_get(ev->atom), ev->target, sel_data->length, sel_data->format);
	FREE(name);
#endif

	Ecore_X_Atom targetAtom = ecore_x_atom_get(ev->target);
	char *stripstr = strndup(sel_data->data, sel_data->length);
	DMSG("get data: %s, len: %d\n", stripstr, strlen(stripstr));
	if (atom_type_index_get(ad, targetAtom) == ATOM_INDEX_IMAGE)
	{
		_add_selection_imagepath(ad, stripstr);
		FREE(stripstr);
	}
	else
		item_add_by_data(ad, targetAtom, stripstr, strlen(stripstr) + 1);

//	FREE(stripstr);

set_clipboard_selection_owner:
	set_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD, NULL);
	if (!is_cbhm_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD))
		xd->selection_timer = ecore_timer_add(SELECTION_CHECK_TIME, selection_timer_cb, ad);

	return ECORE_CALLBACK_DONE;
}

static Eina_Bool _xclient_msg_cb(void *data, int type, void *event)
{
	CALLED();
	AppData *ad = data;
	XHandlerData *xd = ad->xhandler;

	/*	struct _Ecore_X_Event_Client_Message {
		Ecore_X_Window win;
		Ecore_X_Atom   message_type;
		int            format;
		union
		{
			char        b[20];
			short       s[10];
			long        l[5];
		} data;
		Ecore_X_Time   time;
	};*/
	Ecore_X_Event_Client_Message *ev = event;

	if (ev->message_type == xd->atomXKey_MSG)
	{
		DTRACE("XE:ClientMessage for Screen capture\n");
		capture_current_screen(ad);
		return TRUE;
	}

	if (ev->message_type != xd->atomCBHM_MSG)
		return -1;

	DTRACE("## %s\n", ev->data.b);

/*	Atom cbhm_atoms[ITEM_CNT_MAX];
	char atomname[10];
	Ecore_X_Window reqwin = ev->win;*/

	if (strncmp("show", ev->data.b, 4) == 0)
	{
		ad->x_active_win = ev->win;
		if (ev->data.b[4] == '1')
			clipdrawer_paste_textonly_set(ad, EINA_FALSE);
		else
			clipdrawer_paste_textonly_set(ad, EINA_TRUE);

		clipdrawer_activate_view(ad);
	}
	else if (!strcmp("cbhm_hide", ev->data.b))
	{
		clipdrawer_lower_view(ad);
	}
	else if (!strcmp("get count", ev->data.b))
	{
		int icount = item_count_get(ad);
		char countbuf[10];
		DMSG("## cbhm count : %d\n", icount);
		snprintf(countbuf, 10, "%d", icount);
		ecore_x_window_prop_property_set(
				ev->win,
				xd->atomCBHMCount,
				xd->atomUTF8String,
				8,
				countbuf,
				strlen(countbuf)+1);
	}
	/* for OSP */
	else if (strncmp("GET_ITEM", ev->data.b, 8) == 0)
	{
		int itempos = 0;
		int index = 8;
		xd->atomCBHM_ITEM = ecore_x_atom_get("CBHM_ITEM");

		while ('0' <= ev->data.b[index] && ev->data.b[index] <= '9')
		{
			itempos = (itempos * 10) + (ev->data.b[index] - '0');
			index++;
		}

		CNP_ITEM *item = item_get_by_index(ad, itempos);
		if (!item)
		{
			Ecore_X_Atom itemtype = ecore_x_atom_get("CBHM_ERROR");

			char error_buf[] = "OUT OF BOUND";
			int bufsize = sizeof(error_buf);
			ecore_x_window_prop_property_set(
					ev->win,
					xd->atomCBHM_ITEM,
					itemtype,
					8,
					error_buf,
					bufsize);
			DMSG("GET ITEM ERROR msg: %s, index: %d, item count: %d\n",
					ev->data.b, itempos, item_count_get(ad));
		}
		else
		{
			ecore_x_window_prop_property_set(
					ev->win,
					xd->atomCBHM_ITEM,
					ad->targetAtoms[item->type_index].atom[0],
					8,
					item->data,
					item->len);
			DMSG("GET ITEM index: %d, item type: %d, item data: %s, item->len: %d\n",
					itempos, ad->targetAtoms[item->type_index].atom[0],
					item->data, item->len);
		}
	}
	else if (strncmp("SET_ITEM", ev->data.b, 8) == 0)
	{
		int ret = 0;
		int size_ret = 0;
		int num_ret = 0;
		long unsigned int bytes = 0;
		unsigned char *item_data = NULL;
		unsigned char *prop_ret = NULL;
		Ecore_X_Atom format = 0;
		int i;
		xd->atomCBHM_ITEM = ecore_x_atom_get("CBHM_ITEM");
		ret = XGetWindowProperty(ecore_x_display_get(), ad->x_event_win, xd->atomCBHM_ITEM, 0, LONG_MAX, False, ecore_x_window_prop_any_type(),
				(Atom*)&format, &size_ret, &num_ret, &bytes, &prop_ret);
		ecore_x_sync();
		if (ret != Success)
		{
			DMSG("Failed Set Item\n");
			return EINA_FALSE;
		}
		if (!num_ret)
		{
			XFree(prop_ret);
			return EINA_FALSE;
		}

		if (!(item_data = malloc(num_ret * size_ret / 8)))
		{
			XFree(item_data);
			return EINA_FALSE;
		}

		switch (size_ret)
		{
			case 8:
				for (i = 0; i < num_ret; i++)
					item_data[i] = prop_ret[i];
				break;
			case 16:
				for (i = 0; i < num_ret; i++)
					((unsigned short *)item_data)[i] = ((unsigned short *)prop_ret)[i];
				break;
			case 32:
				for (i = 0; i < num_ret; i++)
					((unsigned int *)item_data)[i] = ((unsigned long *)prop_ret)[i];
				break;
		}

		XFree(prop_ret);

		DMSG("item_data:%s format:%s(%d)\n", item_data, ecore_x_atom_name_get(format), format);
		item_add_by_data(ad, format, item_data, strlen(item_data) + 1);
	}
/*	else if (strncmp("get #", ev->data.b, 5) == 0)
	{
		// FIXME : handle greater than 9
		int num = ev->data.b[5] - '0';
		int cur = get_current_history_position();
		num = cur + num - 1;
		if (num > ITEMS_CNT_MAX-1)
			num = num-ITEMS_CNT_MAX;

		if (num >= 0 && num < ITEMS_CNT_MAX)
		{
			DTRACE("## pos : #%d\n", num);
			// FIXME : handle with correct atom
			sprintf(atomname, "CBHM_c%d", num);
			cbhm_atoms[0] = XInternAtom(g_disp, atomname, False);

			CNP_ITEM *item = clipdr;


			if (clipdrawer_get_item_data(ad, num) != NULL)
			{
				XChangeProperty(g_disp, reqwin, cbhm_atoms[0], atomUTF8String,
								8, PropModeReplace,
								(unsigned char *) clipdrawer_get_item_data(ad, num),
								(int) strlen(clipdrawer_get_item_data(ad, num)));
			}
		}
	}
	else if (strcmp("get all", ev->data.b) == 0)
	{
//		print_history_buffer();
		pos = get_current_history_position();
		for (i = 0; i < 5; i++)
		{
			DTRACE("## %d -> %d\n", i, pos);
			sprintf(atomname, "CBHM_c%d", i);
			cbhm_atoms[i] = XInternAtom(g_disp, atomname, False);
			if (clipdrawer_get_item_data(ad, pos) != NULL)
			{
				XChangeProperty(g_disp, reqwin, cbhm_atoms[i], atomUTF8String,
								8, PropModeReplace,
								(unsigned char *) clipdrawer_get_item_data(ad, pos),
								(int) strlen(clipdrawer_get_item_data(ad, pos)));
			}
			pos--;
			if (pos < 0)
				pos = ITEMS_CNT_MAX-1;
		}
	}*/
/*	else if (strcmp("get raw", ev->data.b) == 0)
	{

		if (get_storage_start_addr != NULL)
		{
			XChangeProperty(g_disp, reqwin, atomCBHM_cRAW, atomUTF8String,
							8, PropModeReplace,
							(unsigned char *) get_storage_start_addr(),
							(int) get_total_storage_size());
		}
	}
	*/
	XFlush(ad->x_disp);

	return EINA_TRUE;
}

static Eina_Bool _xfocus_out_cb(void *data, int type, void *event)
{
	CALLED();
	AppData *ad = data;
	DTRACE("XE:FOCUS OUT\n");
	clipdrawer_lower_view(ad);
	return EINA_TRUE;
}

static Eina_Bool _xproperty_notify_cb(void *data, int type, void *event)
{
//	CALLED();
	AppData *ad = data;
	XHandlerData *xd = ad->xhandler;
	ClipdrawerData *cd = ad->clipdrawer;
	Ecore_X_Event_Window_Property *pevent = (Ecore_X_Event_Window_Property *)event;

	if (ad->x_active_win != pevent->win)
		return EINA_TRUE;

	if (xd->atomWindowRotate == pevent->atom)
	{
		int angle = get_active_window_degree(ad->x_active_win);
		if (angle != cd->o_degree)
		{
			cd->o_degree = angle;
			elm_win_rotation_set(cd->main_win, angle);
			set_rotation_to_clipdrawer(cd);
		}
	}

	return EINA_TRUE;
}

static Eina_Bool _xwin_destroy_cb(void *data, int type, void *event)
{
	CALLED();
	AppData *ad = data;
	Ecore_X_Event_Window_Destroy *pevent = event;
	if (ad->x_active_win != pevent->win)
		return EINA_TRUE;
	clipdrawer_lower_view(ad);
}

XHandlerData *init_xhandler(AppData *ad)
{
	XHandlerData *xd = CALLOC(1, sizeof(XHandlerData));
	if (!xd)
		return NULL;
	xd->xsel_clear_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_CLEAR, _xsel_clear_cb, ad);
	xd->xsel_request_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_REQUEST, _xsel_request_cb, ad);
	xd->xsel_notify_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY, _xsel_notify_cb, ad);
	xd->xclient_msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _xclient_msg_cb, ad);
	xd->xfocus_out_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _xfocus_out_cb, ad);
	xd->xproperty_notify_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, _xproperty_notify_cb, ad);
	xd->xwindow_destroy_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY, _xwin_destroy_cb, ad);

	xd->atomInc = ecore_x_atom_get("INCR");
	xd->atomWindowRotate = ecore_x_atom_get("_E_ILLUME_ROTATE_WINDOW_ANGLE");
	xd->atomCBHM_MSG = ecore_x_atom_get("CBHM_MSG");
	xd->atomCBHM_ITEM = ecore_x_atom_get("CBHM_ITEM");
	xd->atomXKey_MSG = ecore_x_atom_get("_XKEY_COMPOSITION");
	xd->atomCBHMCount = ecore_x_atom_get("CBHM_cCOUNT");
	xd->atomUTF8String = ecore_x_atom_get("UTF8_STRING");

	int i;
	for (i = 0; i < ITEM_CNT_MAX; i++)
	{
		char buf[12];
		snprintf(buf, sizeof(buf), "CBHM_ITEM%d", i);
		xd->atomCBHM_ITEM = ecore_x_atom_get(buf);
	}

	return xd;
}

void depose_xhandler(XHandlerData *xd)
{
	ecore_event_handler_del(xd->xsel_clear_handler);
	ecore_event_handler_del(xd->xsel_request_handler);
	ecore_event_handler_del(xd->xsel_notify_handler);
	ecore_event_handler_del(xd->xclient_msg_handler);
	ecore_event_handler_del(xd->xfocus_out_handler);
	ecore_event_handler_del(xd->xproperty_notify_handler);
	ecore_event_handler_del(xd->xwindow_destroy_handler);
	FREE(xd);
}

int get_active_window_degree(Ecore_X_Window active)
{
	//ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE

	int rotation = 0;
	unsigned char *prop_data = NULL;
	int count;
	int ret = ecore_x_window_prop_property_get(
			active, ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE,
			ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
	if (ret && prop_data) memcpy(&rotation, prop_data, sizeof(int));
	if (prop_data) FREE(prop_data);
	return rotation;
}

void slot_property_set(AppData *ad, int index)
{
	XHandlerData *xd = ad->xhandler;

	if (index < 0)
	{
		int i = 0;
		char buf[12];
		CNP_ITEM *item;
		Eina_List *l;

		EINA_LIST_FOREACH(ad->item_list, l, item)
		{
			snprintf(buf, sizeof(buf), "CBHM_ITEM%d", i);
			xd->atomCBHM_ITEM = ecore_x_atom_get(buf);
			if (item)
			{
				ecore_x_window_prop_property_set(
					ad->x_event_win,
					xd->atomCBHM_ITEM,
					ad->targetAtoms[item->type_index].atom[0],
					8,
					item->data,
					item->len);
			DMSG("GET ITEM index: %d, item type: %d, item data: %s, item->len: %d\n",
					i, ad->targetAtoms[item->type_index].atom[0],
					item->data, item->len);
			}

			i++;
		}
	}
	else if (index < ITEM_CNT_MAX)
	{
		char buf[12];
		snprintf(buf, sizeof(buf), "CBHM_ITEM%d", index);
		xd->atomCBHM_ITEM = ecore_x_atom_get(buf);

		CNP_ITEM *item = item_get_by_index(ad, index);
		if (!item)
		{
			Ecore_X_Atom itemtype = ecore_x_atom_get("CBHM_ERROR");

			char error_buf[] = "OUT OF BOUND";
			int bufsize = sizeof(error_buf);
			ecore_x_window_prop_property_set(
					ad->x_event_win,
					xd->atomCBHM_ITEM,
					itemtype,
					8,
					error_buf,
					bufsize);
			DMSG("CBHM Error: index: %d, item count: %d\n",
					index, item_count_get(ad));
		}
		else
		{
			ecore_x_window_prop_property_set(
					ad->x_event_win,
					xd->atomCBHM_ITEM,
					ad->targetAtoms[item->type_index].atom[0],
					8,
					item->data,
					item->len);
			DMSG("GET ITEM index: %d, item type: %d, item data: %s, item->len: %d\n",
					index, ad->targetAtoms[item->type_index].atom[0],
					item->data, item->len);
		}
	}
	else
	{
		DMSG("can't set property\n");
	}
}

void slot_item_count_set(AppData *ad)
{
	XHandlerData *xd = ad->xhandler;

	int icount = item_count_get(ad);
	char countbuf[10];
	snprintf(countbuf, 10, "%d", icount);
	ecore_x_window_prop_property_set(
			ad->x_event_win,
			xd->atomCBHMCount,
			xd->atomUTF8String,
			8,
			countbuf,
			strlen(countbuf)+1);
}
