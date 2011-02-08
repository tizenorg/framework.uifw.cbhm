#include "common.h"
#include "cbhm_main.h"
#include "xcnphandler.h"
#include "storage.h"
#include "clipdrawer.h"
#include "scrcapture.h"

static Ecore_Event_Handler *xsel_clear_handler = NULL;
static Ecore_Event_Handler *xsel_request_handler = NULL;
static Ecore_Event_Handler *xsel_notify_handler = NULL;
static Ecore_Event_Handler *xclient_msg_handler = NULL;
static Ecore_Event_Handler *xfocus_out_handler = NULL;

char *g_lastest_content = NULL;
int g_history_pos = 0;

int xcnp_init(void *data)
{
	struct appdata *ad = data;
	DTRACE("xcnp_init().. start!\n");

	if(!_cbhm_init(ad))
	{
		DTRACE("Failed - _cbhm_init()..!\n");
		return NULL;
	}

	//Adding Event Handlers
	xsel_clear_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_CLEAR, _xsel_clear_cb, ad);
	xsel_request_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_REQUEST, _xsel_request_cb, ad);
	xsel_notify_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY, _xsel_notify_cb, ad);
	xclient_msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _xclient_msg_cb, ad);
	xfocus_out_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _xfocus_out_cb, ad);

	if(!xsel_clear_handler)
		DTRACE("Failed to add ECORE_X_EVENT_SELECTION_CLEAR handler\n");
	if(!xsel_request_handler)
		DTRACE("Failed to add ECORE_X_EVENT_SELECTION_REQUEST handler\n");
	if(!xsel_notify_handler)
		DTRACE("Failed to add ECORE_X_EVENT_SELECTION_NOTIFY handler\n");
	if(!xclient_msg_handler)
		DTRACE("Failed to add ECORE_X_EVENT_CLIENT_MESSAGE handler\n");
	if(!xfocus_out_handler)
		DTRACE("Failed to add ECORE_X_EVENT_WINDOW_FOCUS_OUT handler\n");

	return TRUE;
}

int xcnp_shutdown()
{
	//Removing Event Handlers
	ecore_event_handler_del(xsel_clear_handler);
	ecore_event_handler_del(xsel_request_handler);
	ecore_event_handler_del(xsel_notify_handler);
	ecore_event_handler_del(xclient_msg_handler);
	ecore_event_handler_del(xfocus_out_handler);

	xsel_clear_handler = NULL;
	xsel_request_handler = NULL;
	xsel_notify_handler = NULL;
	xclient_msg_handler = NULL;
	xfocus_out_handler = NULL;

	_cbhm_fini();

	return TRUE;
}

static int _init_atoms()
{
	/* all atoms are global variables */
	atomPrimary = XA_PRIMARY; 
	atomSecondary = XA_SECONDARY; 
//	atomTarget = XA_STRING;
	atomClipboard = XInternAtom(g_disp, ATOM_CLIPBOARD_NAME, False);
	atomCBHM = XInternAtom (g_disp, ATOM_CLIPBOARD_MANAGER_NAME, False);
	atomCBOut = XInternAtom(g_disp, ATOM_CBHM_OUTBUF, False);
	atomInc = XInternAtom(g_disp, "INCR", False);
	atomTargets = XInternAtom(g_disp, "TARGETS", False);
	atomUTF8String = XInternAtom(g_disp, "UTF8_STRING", False);
	atomHtmltext = XInternAtom(g_disp, "text/html;charset=utf-8", False);

	return TRUE;
}

static void _set_cbhmwin_prop()
{
	Atom atomCbhmWin = XInternAtom(g_disp, "CBHM_XWIN", False);
	XChangeProperty(g_disp, g_rootwin, atomCbhmWin, XA_WINDOW, 
					32, PropModeReplace,
					(unsigned char *)&g_evtwin, (int) 1);
}

int increment_current_history_position()
{
	int pos = g_history_pos+1;
	if (pos >= HISTORY_QUEUE_MAX_ITEMS)
		pos = 0;
	g_history_pos = pos;
	return pos;
}

int get_current_history_position()
{
	int pos = g_history_pos-1;
	if (pos < 0)
		pos = HISTORY_QUEUE_MAX_ITEMS-1;
	
	return pos;
}

int add_to_storage_buffer(void *data, char *src, int len)
{
	struct appdata *ad = data;

	if (len <= 0)
		return -1;

	if (g_lastest_content != NULL)
		free(g_lastest_content);
	g_lastest_content = malloc(sizeof(char)*(len+1));
	if (g_history_pos >= HISTORY_QUEUE_MAX_ITEMS)
		g_history_pos = 0;

	// FIXME: remove g_lasteset_content
	memcpy(g_lastest_content, src, len);
	g_lastest_content[len] = '\0';
	adding_item_to_storage(ad, g_history_pos, g_lastest_content);
	increment_current_history_position();

	int nserial = 0;
	nserial = get_storage_serial_code(ad);
	Atom atomCbhmSerial = XInternAtom(g_disp, "CBHM_SERIAL_NUMBER", False);
	XChangeProperty(g_disp, g_evtwin, atomCbhmSerial, XA_INTEGER,
					32, PropModeReplace,
					(unsigned char *)&nserial, (int) 1);
	XFlush(g_disp);

	return 0;
}

int print_storage_buffer(void *data)
{
	struct appdata *ad = data;

	int pos;
	int i = 0;
	for (i = 0; i < HISTORY_QUEUE_MAX_ITEMS; i++)
	{
		pos = get_current_history_position()+i;
		if (pos > HISTORY_QUEUE_MAX_ITEMS-1)
			pos = pos-HISTORY_QUEUE_MAX_ITEMS;
		DTRACE("%d: %s\n", i, 
			   clipdrawer_get_item_data(ad, pos) != NULL ? clipdrawer_get_item_data(ad, pos) : "NULL");
	}
}

int send_convert_selection()
{
	XConvertSelection(g_disp, atomClipboard, atomUTF8String, atomCBOut, g_evtwin, CurrentTime);
	DTRACE("sent convert selection\n");
	return 0;
}

int set_clipboard_manager_owner()
{
	XSetSelectionOwner(g_disp, atomCBHM, g_evtwin, CurrentTime);
	Ecore_X_Window selowner_window = XGetSelectionOwner(g_disp, atomCBHM);
	DTRACE("g_evtwin = 0x%x, setted clipboard manager owner is = 0x%x\n", g_evtwin, selowner_window);
	return 0;
}

int set_selection_owner()
{
	XSetSelectionOwner(g_disp, atomClipboard, g_evtwin, CurrentTime);
	Ecore_X_Window selowner_window = XGetSelectionOwner(g_disp, atomClipboard);
	DTRACE("evtwin = 0x%x, setted selection owner is = 0x%x\n", g_evtwin, selowner_window);
	return 0;
}

int get_selection_content(void *data)
{
	Atom cbtype;
	int cbformat;
	unsigned long cbsize, cbitems;
	unsigned char *cbbuf;
	struct appdata *ad = data;
	const char *unesc;
	size_t unesc_len = 0;
	int i;

	XGetWindowProperty(g_disp, g_evtwin, atomCBOut, 0, 0, False,
					   AnyPropertyType, &cbtype, &cbformat, &cbitems, &cbsize, &cbbuf);
	XFree(cbbuf);

	if (cbtype == atomInc)
	{
		XDeleteProperty(g_disp, g_evtwin, atomCBOut);
		XFlush(g_disp);
		DTRACE("INCR \n");
		return -1;
	}

	DTRACE("cbsize = %d\n", cbsize);

	if (cbformat != 8)
	{
		DTRACE("There're nothing to read = %d\n", cbformat);
		return -2;
	}

	XGetWindowProperty(g_disp, g_evtwin, atomCBOut, 0, (long) cbsize, False,
					   AnyPropertyType, &cbtype, &cbformat, &cbitems, &cbsize, &cbbuf);
	XDeleteProperty(g_disp, g_evtwin, atomCBOut);

#define _NORMAL
#ifdef _NORMAL
	unesc = clipdrawer_get_plain_string_from_escaped(cbbuf);
	if (unesc != NULL)
	{
		unesc_len = strlen(unesc);
		// FIXME: invent more clever way to right trim the string
		for (i = unesc_len-1; i > 0; i--)
		{
			// avoid control characters
			if (unesc[i] >= 0x01 && unesc[i] <= 0x1F)
				continue;
			else
			{
				DTRACE("before right trim len = %d\n", unesc_len);
				unesc_len = i+1;
				DTRACE("after right trim len = %d\n", unesc_len);
				break;
			}
		}
	}
	else
		unesc_len = 0;

#endif

#ifdef _DEMO
	DTRACE("len = %ld, data = %s\n", cbitems, cbbuf);

	if (cbbuf != NULL)
	{
		unesc_len = strlen(cbbuf);
		// FIXME: invent more clever way to right trim the string
		for (i = unesc_len-1; i > 0; i--)
		{
			// avoid control characters
			if (cbbuf[i] >= 0x01 && cbbuf[i] <= 0x1F)
				continue;
			else
			{
				DTRACE("before right trim len = %d\n", unesc_len);
				unesc_len = i+1;
				DTRACE("after right trim len = %d\n", unesc_len);
				break;
			}
		}
	}
	else
		unesc_len = 0;

	if (!strncmp(cbbuf, "file://", 7) && 
		(strcasestr(cbbuf,".png") || strcasestr(cbbuf,".jpg") || strcasestr(cbbuf,".bmp")) &&
		check_regular_file(cbbuf+7))
	{
		DTRACE("clipdrawer add path = %s\n", cbbuf+7);
		clipdrawer_add_item(ad, cbbuf+7, GI_IMAGE);
	}
	else
	{
		add_to_storage_buffer(ad, cbbuf, unesc_len);
		clipdrawer_add_item(ad, cbbuf, GI_TEXT);
	}
	DTRACE("len = %ld, data = %s\n", unesc_len, cbbuf);
#endif

#ifdef _NORMAL
	/* FIXME : it needs two verification. 
               1. does the file exist?
               2. dose the file wanted type? */
	if (!strncmp(unesc, "file://", 7) && 
		(strcasestr(unesc,".png") || strcasestr(unesc,".jpg") || strcasestr(unesc,".bmp")) &&
		check_regular_file(unesc+7))
	{
		DTRACE("clipdrawer add path = %s\n", unesc+7);
		clipdrawer_add_item(unesc+7, GI_IMAGE);
	}
	else
	{
		DTRACE("clipdrawer add string = %s\n", unesc);
		add_to_storage_buffer(ad, unesc, unesc_len);
		clipdrawer_add_item(unesc, GI_TEXT);
	}
	DTRACE("len = %ld, data = %s\n", unesc_len, unesc);
	free(unesc);
#endif

	DTRACE("\n");
	print_storage_buffer(ad);
	DTRACE("\n");

	XFree(cbbuf);

	return 0;
}

int processing_selection_request(Ecore_X_Event_Selection_Request *ev)
{
	XEvent req_evt;
	int req_size;
	Ecore_X_Window req_win;
	Atom req_atom;

	req_size = XExtendedMaxRequestSize(g_disp) / 4;
	if (!req_size)
	{
		req_size = XMaxRequestSize(g_disp) / 4;
	}

    req_win = ev->requestor;
	req_atom = ev->property;

	DTRACE("## wanted target = %d\n", ev->target);
	DTRACE("## wanted target = %s\n", XGetAtomName(g_disp, ev->target));
	DTRACE("## req target atom name = %s\n", XGetAtomName(g_disp, ev->target));

	/* TODO : if there are request which cbhm doesn't understand,
	   then reply None property to requestor */
	/* TODO : add image type */
	if (ev->target == atomTargets) 
	{
//        Atom types[2] = { atomTargets, atomUTF8String };
        Atom types[3] = { atomTargets, atomUTF8String, atomHtmltext };

        // send all (not using INCR) 
        XChangeProperty(g_disp, req_win, req_atom, XA_ATOM,
						32, PropModeReplace, (unsigned char *) types,
						(int) (sizeof(types) / sizeof(Atom)));
		DTRACE("target matched\n");
    }
    else
	{
		DTRACE("target mismatched. trying to txt\n");

		int txt_len;
		if (g_lastest_content != NULL)
			txt_len = strlen(g_lastest_content);
		else
			txt_len = 0;

		if (txt_len > req_size) 
		{
			// INCR 
		}
		else 
		{
			// send all (not using INCR) 
			XChangeProperty(g_disp, req_win, req_atom, atomUTF8String, 
							8, PropModeReplace, (unsigned char *) g_lastest_content, (int) txt_len);
		}
		DTRACE("txt target, len = %d\n", txt_len);
		if (txt_len > 0 && g_lastest_content != NULL)
			DTRACE("txt target, content = %s\n", g_lastest_content);
	}

/*
	DTRACE("wanted window = 0x%x\n", req_win);
	DTRACE("wanted target = %s\n", XGetAtomName(g_disp, ev->target));
*/
	DTRACE("selection target = %s\n", XGetAtomName(g_disp, ev->selection));
	DTRACE("getted atom name = %s\n", XGetAtomName(g_disp, req_atom));
	DTRACE("req target atom name = %s\n", XGetAtomName(g_disp, ev->target));

    req_evt.xselection.property = req_atom;
    req_evt.xselection.type = SelectionNotify;
    req_evt.xselection.display = g_disp;
    req_evt.xselection.requestor = req_win;
    req_evt.xselection.selection = ev->selection;
    req_evt.xselection.target = ev->target;
    req_evt.xselection.time = ev->time;

    XSendEvent(g_disp, ev->requestor, 0, 0, &req_evt);
    XFlush(g_disp);

	return 0;
}

static int _cbhm_init(void *data)
{
	struct appdata *ad = data;
	//Set default data structure
	//Control that the libraries are properly initialized
	if (!ecore_init()) return EXIT_FAILURE;
	if (!ecore_evas_init()) return EXIT_FAILURE;
	if (!edje_init()) return EXIT_FAILURE;

	// do not x init in this module, it disconnect previous e17's X connection
	//ecore_x_init(NULL);

	g_disp = ecore_x_display_get();

	if( !g_disp )
	{
		DTRACE("Failed to get display\n");
		return -1;
	}

	g_rootwin = DefaultRootWindow(g_disp);
	g_evtwin = ecore_x_window_new(g_rootwin, 0, 0, 19, 19);
	ecore_x_netwm_name_set(g_evtwin, CLIPBOARD_MANAGER_WINDOW_TITLE_STRING);

	XSelectInput(g_disp, g_evtwin, PropertyChangeMask);
	XSelectInput(g_disp, g_rootwin, StructureNotifyMask);
//	ecore_x_window_show(g_evtwin);
	ecore_x_flush();

	_set_cbhmwin_prop();
    _init_atoms();
	init_storage(ad);

	DTRACE("_cbhm_init ok\n");

	set_clipboard_manager_owner();
	send_convert_selection();
	set_selection_owner();

	return TRUE;
}

static void _cbhm_fini()
{
	struct appdata *ad = g_get_main_appdata();

	close_storage(ad);

	return;
}

static int _xsel_clear_cb(void *data, int ev_type, void *event)
{
	struct appdata *ad = data;

	Ecore_X_Event_Selection_Clear *ev = (Ecore_X_Event_Selection_Clear *)event;

	if (ev->selection != ECORE_X_SELECTION_CLIPBOARD)
		return TRUE;
	
	DTRACE("XE:SelectionClear\n");

	send_convert_selection();
	ecore_x_flush();
	/* TODO : set selection request is should after convert selection
	 * is done */
	set_selection_owner();

	return TRUE;
}

static int _xsel_request_cb(void *data, int ev_type, void *event)
{
	Ecore_X_Event_Selection_Request *ev = (Ecore_X_Event_Selection_Request *)event;

	if (ev->selection != atomClipboard)
		return TRUE;

	DTRACE("XE:SelectionRequest\n");

	processing_selection_request(ev);

	return TRUE;
}

static int _xsel_notify_cb(void *data, int ev_type, void *event)
{
	struct appdata *ad = data;

	Ecore_X_Event_Selection_Notify *ev = (Ecore_X_Event_Selection_Notify *)event;
	Ecore_X_Selection_Data_Text *text_data = NULL;

	if (ev->selection != ECORE_X_SELECTION_CLIPBOARD)
		return TRUE;
	
	DTRACE("XE:SelectionNotify\n");

	text_data = ev->data;
	DTRACE("content type = %d\n", text_data->data.content);
	get_selection_content(ad);

	if (text_data->data.content != ECORE_X_SELECTION_CONTENT_TEXT)
	{
		DTRACE("content isn't text\n");
	}

	return TRUE;
}

static int _xclient_msg_cb(void *data, int ev_type, void *event)
{
	struct appdata *ad = data;

	Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message*)event;

	Atom atomCBHM_MSG = XInternAtom(g_disp, "CBHM_MSG", False);
	Atom atomCBHM_cRAW = XInternAtom(g_disp, "CBHM_cRAW", False);
	Atom atomXKey_MSG = XInternAtom(g_disp, "_XKEY_COMPOSITION", False);
	char atomname[10];
	Atom cbhm_atoms[HISTORY_QUEUE_MAX_ITEMS];
	Ecore_X_Window reqwin = ev->win;
	int i, pos;

	if (ev->message_type == atomXKey_MSG)
	{
		DTRACE("XE:ClientMessage for Screen capture\n");

		capture_current_screen(ad);

		return TRUE;
	}

	if (ev->message_type != atomCBHM_MSG)
		return -1;

	DTRACE("XE:ClientMessage for CBHM\n");

	DTRACE("## %s\n", ev->data.b);

	if (strcmp("get count", ev->data.b) == 0)
	{
		int icount = get_item_counts(ad);
		char countbuf[10];
		DTRACE("## cbhm count : %d\n", icount);
		sprintf(countbuf, "%d", icount);
		sprintf(atomname, "CBHM_cCOUNT");
		cbhm_atoms[0] = XInternAtom(g_disp, atomname, False);
		XChangeProperty(g_disp, reqwin, cbhm_atoms[0], atomUTF8String, 
						8, PropModeReplace, 
						(unsigned char *) countbuf, 
						(int) strlen(countbuf));
	
	}
	else if (strncmp("get #", ev->data.b, 5) == 0)
	{
		// FIXME : handle greater than 9
		int num = ev->data.b[5] - '0';
		int cur = get_current_history_position();
		num = cur + num - 1;
		if (num > HISTORY_QUEUE_MAX_ITEMS-1)
			num = num-HISTORY_QUEUE_MAX_ITEMS;

		if (num >= 0 && num < HISTORY_QUEUE_MAX_ITEMS)
		{
			DTRACE("## pos : #%d\n", num);
			// FIXME : handle with correct atom
			sprintf(atomname, "CBHM_c%d", num);
			cbhm_atoms[0] = XInternAtom(g_disp, atomname, False);
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
				pos = HISTORY_QUEUE_MAX_ITEMS-1;
		}
	}
	else if (strcmp("get raw", ev->data.b) == 0)
	{
/*
		if (get_storage_start_addr != NULL)
		{
			XChangeProperty(g_disp, reqwin, atomCBHM_cRAW, atomUTF8String, 
							8, PropModeReplace, 
							(unsigned char *) get_storage_start_addr(),
							(int) get_total_storage_size());
		}
*/
	}
	else if (strncmp("show", ev->data.b, 4) == 0)
	{
		if (ev->data.b[4] != NULL && ev->data.b[4] == '1')
			clipdrawer_paste_textonly_set(ad, EINA_FALSE);
		else
			clipdrawer_paste_textonly_set(ad, EINA_TRUE);

		clipdrawer_activate_view(ad);
	}

	XFlush(g_disp);

	return TRUE;
}

static int _xfocus_out_cb(void *data, int ev_type, void *event)
{
	struct appdata *ad = data;

	DTRACE("XE:FOCUS OUT\n");

	clipdrawer_lower_view(ad);

	return TRUE;
}

void set_transient_for(void *data)
{
	struct appdata *ad = data;

	Ecore_X_Window xwin_active = None;
	Atom atomActive = XInternAtom(g_disp, "_NET_ACTIVE_WINDOW", False);

	if (ecore_x_window_prop_window_get(DefaultRootWindow(g_disp), 
									   atomActive, &xwin_active, 1) != -1)
	{
		ecore_x_icccm_transient_for_set (elm_win_xwindow_get(ad->win_main), xwin_active);
		DTRACE("Success to set transient_for active window = 0x%X\n", xwin_active);
	}
	else
	{
		DTRACE("Failed to find active window for transient_for\n");
	}
}

void unset_transient_for(void *data)
{
	struct appdata *ad = data;

	ecore_x_icccm_transient_for_unset(elm_win_xwindow_get(ad->win_main));
}

static Ecore_X_Window get_selection_secondary_target_win()
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop_return = NULL;
	Atom atomCbhmXTarget = XInternAtom(g_disp, "CBHM_XTARGET", False);
	static Ecore_X_Window xtarget = None;
	if (xtarget != None)
		return xtarget;

	if(Success == 
	   XGetWindowProperty(g_disp, DefaultRootWindow(g_disp), atomCbhmXTarget, 
						  0, sizeof(Ecore_X_Window), False, XA_WINDOW, 
						  &actual_type, &actual_format, &nitems, &bytes_after, &prop_return) && 
	   prop_return)
	{
		xtarget = *(Ecore_X_Window*)prop_return;
		XFree(prop_return);
		fprintf(stderr, "## find clipboard secondary target at root\n");
	}
	return xtarget;
}

int set_selection_secondary_data(char *sdata)
{
	Ecore_X_Window setwin = get_selection_secondary_target_win();
	if (setwin == None)
		return 0;

	if (sdata == NULL)
		return 0;

	int slen = strlen(sdata);

	fprintf(stderr, "## cbhm xwin = 0x%x, d = %s\n", setwin, sdata);

	ecore_x_selection_secondary_set(setwin, sdata, slen);
}
