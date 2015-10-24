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

Eina_Bool cbhm_send_event(AppData *ad, Ecore_X_Window xwin, char *msg)
{
	ClipdrawerData *cd = ad->clipdrawer;

	Ecore_X_Atom x_atom_cbhm_msg = ecore_x_atom_get("CBHM_MSG");
	XClientMessageEvent m;
	memset(&m, 0, sizeof(m));
	m.type = ClientMessage;
	m.display = ecore_x_display_get();
	m.window = cd->x_main_win;
	m.message_type = x_atom_cbhm_msg;
	m.format = 8;
	snprintf(m.data.b, 20, "%s", msg);

	XSendEvent(ecore_x_display_get(), xwin, False, NoEventMask, (XEvent*)&m);

	ecore_x_sync();

	return EINA_TRUE;
}

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
	DBG("selection_owner: 0x%x, x_event_win: 0x%x", sel_owner, ad->x_event_win);
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

	Eina_Bool (*selection_func)(Ecore_X_Window win, const void *data, int size) = NULL;

	switch(selection)
	{
		case ECORE_X_SELECTION_SECONDARY:
//			ecore_x_selection_secondary_clear();
			selection_func = ecore_x_selection_secondary_set;
			break;
		case ECORE_X_SELECTION_CLIPBOARD:
//			ecore_x_selection_clipboard_clear();
			selection_func = ecore_x_selection_clipboard_set;
			break;
		default:
			return EINA_FALSE;
	}

	slot_selected_item_set(ad);
	if (selection_func(ad->x_event_win, NULL, 0))
		return EINA_TRUE;

	ERR("set selection failed");
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
	ClipdrawerData *cd = ad->clipdrawer;
	XHandlerData *xd = ad->xhandler;
	Ecore_X_Event_Selection_Clear *ev = event;

	DBG("in %s, ev->win: 0x%x\n", __func__, ev->win);

	if (is_cbhm_selection_owner(ad, ev->selection)) return EINA_TRUE;

	if (ev->selection == ECORE_X_SELECTION_SECONDARY && cd->item_clicked)
	{
		cd->item_clicked = EINA_FALSE;
		set_selection_owner(ad, ECORE_X_SELECTION_SECONDARY, NULL);
	}

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
#ifdef MDM_ENABLE
	if (!data || !event || !_mdm_get_allow_clipboard()) return ECORE_CALLBACK_PASS_ON;
#else
	if (!data || !event) return ECORE_CALLBACK_PASS_ON;
#endif
	AppData *ad = data;
	Ecore_X_Event_Selection_Request *ev = event;

	char *names[3];
	DBG("selection_owner: 0x%x, ev->... owner: 0x%x, req: 0x%x, selection: %s, target: %s, property: %s",
			get_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD), ev->owner, ev->requestor,
			names[0] = ecore_x_atom_name_get(ev->selection),
			names[1] = ecore_x_atom_name_get(ev->target),
			names[2] = ecore_x_atom_name_get(ev->property));
	FREE(names[0]);
	FREE(names[1]);
	FREE(names[2]);

	CNP_ITEM *item = NULL;
	if (ev->selection == ECORE_X_ATOM_SELECTION_CLIPBOARD)
		item = item_get_last(ad);
	else if (ev->selection == ECORE_X_ATOM_SELECTION_SECONDARY)
		item = ad->clip_selected_item;
	else
		return ECORE_CALLBACK_PASS_ON;

	if (!item)
	{
		DBG("has no item");
		ecore_x_selection_notify_send(ev->requestor,
				ev->selection,
				None,
				None,
				CurrentTime);
		DBG("change property notify");
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
		DBG("converter return FALSE");
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
		DBG("change property");
	}

	ecore_x_selection_notify_send(ev->requestor,
			ev->selection,
			ev->target,
			property,
			CurrentTime);
	DBG("change property notify");
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

	DBG("targets_data->num_targets: 0x%x", targets_data->num_targets);
	int i, j, k;
	for (j = 0; j < targets_data->num_targets; j++)
	{
		for (i = 0; i < ATOM_INDEX_MAX; i++)
		{
			DBG("get target: %s", targets_data->targets[j]);
			for (k = 0; k < ad->targetAtoms[i].atom_cnt; k++)
			{
				if (!SAFE_STRCMP(targets_data->targets[j], ad->targetAtoms[i].name[k]))
				{
					DBG("find matched target: %s", ad->targetAtoms[i].name[k]);
					ecore_x_selection_clipboard_request(ad->x_event_win, ad->targetAtoms[i].name[k]);
					return;
				}
			}
		}
	}
	ERR("get target atom failed");
}

static Eina_Bool _add_selection_imagepath(AppData* ad, char *str)
{
	if (!ad || !str)
		return EINA_FALSE;
	DBG("get FILE: %s", str);
	char *slash = SAFE_STRCHR(str, '/');
	while (slash && slash[0] == '/')
	{
		if (slash[1] != '/')
		{
			char *filepath;
			filepath = SAFE_STRDUP(slash);
			if (filepath)
			{
				if (ecore_file_exists(filepath))
				{
					item_add_by_data(ad, ad->targetAtoms[ATOM_INDEX_IMAGE].atom[0], filepath, SAFE_STRLEN(filepath) + 1, EINA_TRUE);
					return EINA_TRUE;
				}
				else
					FREE(filepath);
			}
			break;
		}
		slash++;
	}
	ERR("it isn't normal file = %s", str);
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
			DBG("ECORE_X_SELECTION_CONTENT_NONE");
			break;
		case ECORE_X_SELECTION_CONTENT_TEXT:
			DBG("ECORE_X_SELECTION_CONTENT_TEXT");
		/*	struct _Ecore_X_Selection_Data_Text {
				Ecore_X_Selection_Data data;
				char                  *text;
			};
			Ecore_X_Selection_Data_Text *text_data = ev->data;*/
		//	DBG("sel_data->data: 0x%x, text_data->text: 0x%x", sel_data->data, text_data->text);
			break;
		case ECORE_X_SELECTION_CONTENT_FILES:
			DBG("ECORE_X_SELECTION_CONTENT_FILES");
			_get_selection_data_files(ad, ev->data);
			goto set_clipboard_selection_owner;
			break;
		case ECORE_X_SELECTION_CONTENT_TARGETS:
			DBG("ECORE_X_SELECTION_CONTENT_TARGETS");
			send_convert_selection_target(ad, ev->data);
			if (!is_cbhm_selection_owner(ad, ECORE_X_SELECTION_CLIPBOARD))
				xd->selection_timer = ecore_timer_add(SELECTION_CHECK_TIME, selection_timer_cb, ad);
			return ECORE_CALLBACK_DONE;
		case ECORE_X_SELECTION_CONTENT_CUSTOM:
			DBG("ECORE_X_SELECTION_CONTENT_CUSTOM");
			break;
	}
	char *name;
	DBG("get atom: %d(%s), target: %s, length: %d, format: %d",
			ev->atom, name = ecore_x_atom_name_get(ev->atom), ev->target, sel_data->length, sel_data->format);
	FREE(name);
#ifdef MDM_ENABLE
	if (_mdm_get_allow_clipboard())
	{
#else
	{
#endif
		Ecore_X_Atom targetAtom = ecore_x_atom_get(ev->target);
		char *stripstr = SAFE_STRNDUP((char *)sel_data->data, sel_data->length);
		DBG("get data: %s, len: %d", stripstr, SAFE_STRLEN(stripstr));
		if (atom_type_index_get(ad, targetAtom) == ATOM_INDEX_IMAGE)
		{
			_add_selection_imagepath(ad, stripstr);
			FREE(stripstr);
		}
		else
			item_add_by_data(ad, targetAtom, stripstr, SAFE_STRLEN(stripstr) + 1, EINA_TRUE);
	}
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
	if (!data || !event) return ECORE_CALLBACK_PASS_ON;

	AppData *ad = data;
	XHandlerData *xd = ad->xhandler;
	Ecore_X_Event_Client_Message *ev = event;

	if (ev->message_type != xd->atomCBHM_MSG)
		return -1;

	DBG("## %s", ev->data.b);

/*	Atom cbhm_atoms[ITEM_CNT_MAX];
	char atomname[10];
	Ecore_X_Window reqwin = ev->win;*/

	if (!SAFE_STRCMP("set_owner", ev->data.b))
	{
		cbhm_send_event(ad, ev->win, "SET_OWNER");
	}
	else if (!SAFE_STRNCMP("show", ev->data.b, 4))
	{
#ifdef MDM_ENABLE
		if (_mdm_get_allow_clipboard())
		{
#endif
			ad->x_active_win = ev->win;
			if (ev->data.b[4] == '1')
				clipdrawer_paste_textonly_set(ad, EINA_FALSE);
			else
				clipdrawer_paste_textonly_set(ad, EINA_TRUE);

			Ecore_X_Illume_Clipboard_State state = ecore_x_e_illume_clipboard_state_get(ad->x_active_win);
			if(state != ECORE_X_ILLUME_CLIPBOARD_STATE_ON)
				clipdrawer_activate_view(ad);

#ifdef MDM_ENABLE
		}
#endif
	}
	else if (!SAFE_STRCMP("cbhm_hide", ev->data.b))
	{
		Ecore_X_Illume_Clipboard_State state = ecore_x_e_illume_clipboard_state_get(ad->x_active_win);
		if(state == ECORE_X_ILLUME_CLIPBOARD_STATE_ON)
		{
			clipdrawer_lower_view(ad);
		}
	}
	else if (!SAFE_STRCMP("get count", ev->data.b))
	{
		int i, icount;
		char countbuf[10];

		for (i = 0; i < ATOM_INDEX_COUNT_MAX; i++)
		{
			icount = item_count_get(ad, i);
			snprintf(countbuf, 10, "%d", icount);
			ecore_x_window_prop_property_set(
					ev->win,
					xd->atomCBHMCount[i],
					xd->atomUTF8String,
					8,
					countbuf,
					strlen(countbuf)+1);
		}
	}
	/* for OSP */
	else if (!SAFE_STRNCMP("GET_ITEM", ev->data.b, 8))
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
			DBG("GET ITEM ERROR msg: %s, index: %d, item count: %d",
					ev->data.b, itempos, item_count_get(ad, ATOM_INDEX_COUNT_ALL));
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
			DBG("GET ITEM index: %d, item type: %d, item data: %s, item->len: %d",
					itempos, ad->targetAtoms[item->type_index].atom[0],
					(char *)item->data, item->len);
		}
	}
	else if (!SAFE_STRNCMP("SET_ITEM", ev->data.b, 8))
	{
		int ret = 0;
		int size_ret = 0;
		unsigned long num_ret = 0;
		unsigned long bytes = 0;
		unsigned char *item_data = NULL;
		unsigned char *prop_ret = NULL;
		Ecore_X_Atom format = 0;
		int i;
		char *format_name = NULL;
		xd->atomCBHM_ITEM = ecore_x_atom_get("CBHM_ITEM");
		ret = XGetWindowProperty(ecore_x_display_get(), ad->x_event_win, xd->atomCBHM_ITEM, 0, LONG_MAX, False, ecore_x_window_prop_any_type(),
				(Atom*)&format, &size_ret, &num_ret, &bytes, &prop_ret);
		ecore_x_sync();
		if (ret != Success)
		{
			DBG("Failed Set Item");
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
			default:
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

		format_name = ecore_x_atom_name_get(format);
		DBG("item_data:%s format:%s(%d)\n", item_data, format_name, format);
		free(format_name);
		item_add_by_data(ad, format, item_data, SAFE_STRLEN((char *)item_data) + 1, EINA_TRUE);
	}
	else if (!SAFE_STRNCMP("DEL_ITEM", ev->data.b, 8))
	{
		int itempos = 0;
		int index = 8;

		while ('0' <= ev->data.b[index] && ev->data.b[index] <= '9')
		{
			itempos = (itempos * 10) + (ev->data.b[index] - '0');
			index++;
		}

		item_delete_by_index(ad, itempos);
	}
/*	else if (!SAFE_STRNCMP("get #", ev->data.b, 5))
	{
		// FIXME : handle greater than 9
		int num = ev->data.b[5] - '0';
		int cur = get_current_history_position();
		num = cur + num - 1;
		if (num > ITEMS_CNT_MAX-1)
			num = num-ITEMS_CNT_MAX;

		if (num >= 0 && num < ITEMS_CNT_MAX)
		{
			DBG("## pos : #%d", num);
			// FIXME : handle with correct atom
			sprintf(atomname, "CBHM_c%d", num);
			cbhm_atoms[0] = XInternAtom(g_disp, atomname, False);

			CNP_ITEM *item = clipdr;


			if (clipdrawer_get_item_data(ad, num) != NULL)
			{
				XChangeProperty(g_disp, reqwin, cbhm_atoms[0], atomUTF8String,
								8, PropModeReplace,
								(unsigned char *) clipdrawer_get_item_data(ad, num),
								(int) SAFE_STRLEN(clipdrawer_get_item_data(ad, num)));
			}
		}
	}
	else if (!SAFE_STRCMP("get all", ev->data.b))
	{
//		print_history_buffer();
		pos = get_current_history_position();
		for (i = 0; i < 5; i++)
		{
			DBG("## %d -> %d", i, pos);
			sprintf(atomname, "CBHM_c%d", i);
			cbhm_atoms[i] = XInternAtom(g_disp, atomname, False);
			if (clipdrawer_get_item_data(ad, pos) != NULL)
			{
				XChangeProperty(g_disp, reqwin, cbhm_atoms[i], atomUTF8String,
								8, PropModeReplace,
								(unsigned char *) clipdrawer_get_item_data(ad, pos),
								(int) SAFE_STRLEN(clipdrawer_get_item_data(ad, pos)));
			}
			pos--;
			if (pos < 0)
				pos = ITEMS_CNT_MAX-1;
		}
	}*/
/*	else if (!SAFE_STRCMP("get raw", ev->data.b))
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
	if (!data || !event) return ECORE_CALLBACK_PASS_ON;

	AppData *ad = data;
	DBG("XE:FOCUS OUT");
	clipdrawer_lower_view(ad);
	return EINA_TRUE;
}

static Eina_Bool _xproperty_notify_cb(void *data, int type, void *event)
{
    if (!data || !event) return ECORE_CALLBACK_PASS_ON;

	AppData *ad = data;
	XHandlerData *xd = ad->xhandler;
	Ecore_X_Event_Window_Property *pevent = (Ecore_X_Event_Window_Property *)event;
	char *filepath;

	//Screen captured.
	if (pevent->win == ad->x_event_win)
	{
		if (pevent->atom == xd->atomShotString)
		{
			filepath = ecore_x_window_prop_string_get(ad->x_event_win, xd->atomShotString);
			if (filepath && ecore_file_exists(filepath))
			{
				DBG("captured item added filepath = %s", filepath);
				item_add_by_data(ad, ad->targetAtoms[ATOM_INDEX_IMAGE].atom[0], filepath, SAFE_STRLEN(filepath) + 1, EINA_FALSE);
			}
			else FREE(filepath);
		}
	}

	return EINA_TRUE;
}

static Eina_Bool _xwin_destroy_cb(void *data, int type, void *event)
{
	CALLED();
	if (!data || !event) return ECORE_CALLBACK_PASS_ON;

	AppData *ad = data;
	Ecore_X_Event_Window_Destroy *pevent = event;
	if (ad->x_active_win != pevent->win)
		return ECORE_CALLBACK_PASS_ON;
	clipdrawer_lower_view(ad);
	return ECORE_CALLBACK_DONE;
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
	xd->atomShotString = ecore_x_atom_get("_E_SHOT_IMG_FILEPATH");
	xd->atomCBHM_MSG = ecore_x_atom_get("CBHM_MSG");
	xd->atomCBHM_ITEM = ecore_x_atom_get("CBHM_ITEM");
	xd->atomCBHMCount[ATOM_INDEX_COUNT_ALL] = ecore_x_atom_get("CBHM_cCOUNT");
	xd->atomCBHMCount[ATOM_INDEX_COUNT_TEXT] = ecore_x_atom_get("CBHM_TEXT_cCOUNT");
	xd->atomCBHMCount[ATOM_INDEX_COUNT_IMAGE] = ecore_x_atom_get("CBHM_IMAGE_cCOUNT");
	xd->atomUTF8String = ecore_x_atom_get("UTF8_STRING");
	xd->atomCBHM_SELECTED_ITEM = ecore_x_atom_get("CBHM_SELECTED_ITEM");

	int i;
	for (i = 0; i < ITEM_CNT_MAX; i++)
	{
		char buf[20];
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
			DBG("GET ITEM index: %d, item type: %d, item data: %s, item->len: %d",
					i, ad->targetAtoms[item->type_index].atom[0],
					(char *)item->data, item->len);
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
			DBG("CBHM Error: index: %d, item count: %d",
					index, item_count_get(ad, ATOM_INDEX_COUNT_ALL));
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
			DBG("GET ITEM index: %d, item type: %d, item data: %s, item->len: %d",
					index, ad->targetAtoms[item->type_index].atom[0],
					(char *)item->data, item->len);
		}
	}
	else
	{
		DBG("can't set property");
	}
}

void slot_item_count_set(AppData *ad)
{
	XHandlerData *xd = ad->xhandler;
	int i, icount;
	char countbuf[10];

	for (i = 0; i < ATOM_INDEX_COUNT_MAX; i++)
	{
		icount = item_count_get(ad, i);
		snprintf(countbuf, 10, "%d", icount);
		ecore_x_window_prop_property_set(
				ad->x_event_win,
				xd->atomCBHMCount[i],
				xd->atomUTF8String,
				8,
				countbuf,
				strlen(countbuf)+1);
	}
}

void slot_selected_item_set(AppData *ad)
{
	XHandlerData *xd = ad->xhandler;
	CNP_ITEM *item = ad->clip_selected_item;

	if (item)
	{
		char buf[20];
		snprintf(buf, sizeof(buf), "CBHM_SELECTED_ITEM");
		xd->atomCBHM_SELECTED_ITEM = ecore_x_atom_get(buf);

		ecore_x_window_prop_property_set(
				ad->x_event_win,
				xd->atomCBHM_SELECTED_ITEM,
				ad->targetAtoms[item->type_index].atom[0],
				8,
				item->data,
				item->len);
	}
	else
	{
		Ecore_X_Atom itemtype = ecore_x_atom_get("CBHM_ERROR");

		char error_buf[] = "NOT EXIST SELECTED ITEM";
		int bufsize = sizeof(error_buf);
		ecore_x_window_prop_property_set(
				ad->x_event_win,
				xd->atomCBHM_SELECTED_ITEM,
				itemtype,
				8,
				error_buf,
				bufsize);

	}
}
#ifdef MDM_ENABLE
#ifdef MDM_PHASE_2
static void _popup_timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *win = data;
	evas_object_del(obj);
	evas_object_del(win);
}
#endif
Eina_Bool _mdm_get_allow_clipboard()
{
#ifdef MDM_PHASE_2
	Eina_Bool result = EINA_TRUE;
	if (mdm_get_service() == MDM_RESULT_SUCCESS)
	{
		ret = mdm_get_allow_clipboard();
		if (ret == MDM_RESTRICTED)
		{
			Evas_Object *win = elm_win_add(NULL, "cbhm popup", ELM_WIN_DIALOG_BASIC);
			int w,h;
			ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
			evas_object_resize(win, w, h);
			elm_win_alpha_set(win, EINA_TRUE);
			evas_object_show(win);

			Evas_Object *popup;
			popup = elm_popup_add(win);

			evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			elm_object_text_set(popup, "Security policy restricts using clipboard");
			elm_popup_timeout_set(popup, 2.0);
			evas_object_smart_callback_add(popup, "timeout", _popup_timeout_cb, win);
			evas_object_show(popup);

			result = EINA_FALSE;
		}
		else
			result = EINA_TRUE;
	}
	else
		return EINA_TRUE;

	mdm_release_service();

	return result;
#else
	return EINA_TRUE;
#endif
}
#endif
