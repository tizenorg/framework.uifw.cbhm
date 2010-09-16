#include "common.h"
#include "cbhm_main.h"
#include "xcnphandler.h"
#include "storage.h"
#include "clipdrawer.h"

static Ecore_Event_Handler *xsel_clear_handler = NULL;
static Ecore_Event_Handler *xsel_request_handler = NULL;
static Ecore_Event_Handler *xsel_notify_handler = NULL;
static Ecore_Event_Handler *xclient_msg_handler = NULL;

char *g_lastest_content = NULL;
int g_history_pos = 0;

int xcnp_init(void *data)
{
	struct appdata *ad = data;
	DTRACE("xcnp_init().. start!\n");

	if(!_cbhm_init())
	{
		DTRACE("Failed - _cbhm_init()..!\n");
		return NULL;
	}

	//Adding Event Handlers
	xsel_clear_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_CLEAR, _xsel_clear_cb, ad);
	xsel_request_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_REQUEST, _xsel_request_cb, ad);
	xsel_notify_handler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY, _xsel_notify_cb, ad);
	xclient_msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _xclient_msg_cb, ad);

	if(!xsel_clear_handler)
		DTRACE("Failed to add ECORE_X_EVENT_SELECTION_CLEAR handler\n");
	if(!xsel_request_handler)
		DTRACE("Failed to add ECORE_X_EVENT_SELECTION_REQUEST handler\n");
	if(!xsel_notify_handler)
		DTRACE("Failed to add ECORE_X_EVENT_SELECTION_NOTIFY handler\n");
	if(!xclient_msg_handler)
		DTRACE("Failed to add ECORE_X_EVENT_CLIENT_MESSAGE handler\n");

	return TRUE;
}

int xcnp_shutdown()
{
	//Removing Event Handlers
	ecore_event_handler_del(xsel_clear_handler);
	ecore_event_handler_del(xsel_request_handler);
	ecore_event_handler_del(xsel_notify_handler);
	ecore_event_handler_del(xclient_msg_handler);

	xsel_clear_handler = NULL;
	xsel_request_handler = NULL;
	xsel_notify_handler = NULL;
	xclient_msg_handler = NULL;

	_cbhm_fini();

	return TRUE;
}

static int _init_atoms()
{
	/* all atoms are global variables */
	atomPrimary = XA_PRIMARY; 
	atomSecondary = XA_SECONDARY; 
	atomTarget = XA_STRING;
	atomClipboard = XInternAtom(g_disp, ATOM_CLIPBOARD_NAME, False);
	atomCBHM = XInternAtom (g_disp, ATOM_CLIPBOARD_MANAGER_NAME, False);
	atomCBOut = XInternAtom(g_disp, ATOM_CBHM_OUTBUF, False);
	atomInc = XInternAtom(g_disp, "INCR", False);
	atomTargets = XInternAtom(g_disp, "TARGETS", False);
	atomUTF8String = XInternAtom(g_disp, "UTF8_STRING", False);

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
	if (pos >= HISTORY_QUEUE_NUMBER)
		pos = 0;
	g_history_pos = pos;
	return pos;
}

int get_current_history_position()
{
	int pos = g_history_pos-1;
	if (pos < 0)
		pos = HISTORY_QUEUE_NUMBER;
	
	return pos;
}

int add_to_storage_buffer(void *data, char *src, int len)
{
	struct appdata *ad = data;

	if (len <= 0)
		return -1;

	if (g_lastest_content == NULL)
		g_lastest_content = malloc(sizeof(char)*(4*1024));
	if (g_history_pos >= HISTORY_QUEUE_NUMBER)
		g_history_pos = 0;

	// FIXME: remove g_lasteset_content
	strcpy(g_lastest_content, src);
	adding_item_to_storage(g_history_pos, g_lastest_content);
	increment_current_history_position();

	int nserial = 0;
	nserial = get_storage_serial_code();
	Atom atomCbhmSerial = XInternAtom(g_disp, "CBHM_SERIAL_NUMBER", False);
	XChangeProperty(g_disp, g_evtwin, atomCbhmSerial, XA_INTEGER,
					32, PropModeReplace,
					(unsigned char *)&nserial, (int) 1);
	XFlush(g_disp);

	clipdrawer_update_contents(ad);

	return 0;
}

int print_storage_buffer()
{
	int pos;
	int i = 0;
	for (i = 0; i < HISTORY_QUEUE_NUMBER; i++)
	{
		pos = get_current_history_position()+i;
		if (pos > HISTORY_QUEUE_NUMBER-1)
			pos = pos-HISTORY_QUEUE_NUMBER;
		DTRACE("%d: %s\n", i, get_item_contents_by_pos(pos) != NULL ? get_item_contents_by_pos(pos) : "NULL");
	}
}

int send_convert_selection()
{
	XConvertSelection(g_disp, atomClipboard, atomTarget, atomCBOut, g_evtwin, CurrentTime);
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

	add_to_storage_buffer(ad, cbbuf, cbitems);
	DTRACE("len = %ld, data = %s\n", cbitems, cbbuf);

	DTRACE("\n");
	print_storage_buffer();
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

	/* TODO : if there are request which cbhm doesn't understand,
	   then reply None property to requestor */
	if (ev->target == atomTargets) 
	{
        Atom types[2] = { atomTargets, atomTarget };

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
			XChangeProperty(g_disp, req_win, req_atom, atomTarget, 
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

static int _cbhm_init()
{
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
//	ecore_x_window_show(g_evtwin);
	ecore_x_flush();

	_set_cbhmwin_prop();
    _init_atoms();
	init_storage();

	DTRACE("_cbhm_init ok\n");

	set_clipboard_manager_owner();
	send_convert_selection();
	set_selection_owner();

	return TRUE;
}

static void _cbhm_fini()
{
	close_storage();

	return;
}

static int _xsel_clear_cb(void *data, int ev_type, void *event)
{
	struct appdata *ad = data;

	Ecore_X_Event_Selection_Clear *ev = (Ecore_X_Event_Selection_Clear *)event;

	if (ev->selection != ECORE_X_SELECTION_CLIPBOARD)
		return TRUE;
	
	DTRACE("SelectionClear\n");

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

	DTRACE("SelectionRequest\n");

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
	
	DTRACE("SelectionNotify\n");

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
	char atomname[10];
	Atom cbhm_atoms[HISTORY_QUEUE_NUMBER];
	Ecore_X_Window reqwin = ev->win;
	int i, pos;

	if (ev->message_type != atomCBHM_MSG)
		return -1;

	DTRACE("ClientMessage for CBHM\n");

	DTRACE("## %s\n", ev->data.b);

	if (strcmp("get count", ev->data.b) == 0)
	{
		int icount = get_item_counts();
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
		if (num > HISTORY_QUEUE_NUMBER-1)
			num = num-HISTORY_QUEUE_NUMBER;

		if (num >= 0 && num < HISTORY_QUEUE_NUMBER)
		{
			DTRACE("## pos : #%d\n", num);
			// FIXME : handle with correct atom
			sprintf(atomname, "CBHM_c%d", num);
			cbhm_atoms[0] = XInternAtom(g_disp, atomname, False);
			if (get_item_contents_by_pos(num) != NULL)
			{
				XChangeProperty(g_disp, reqwin, cbhm_atoms[0], atomUTF8String, 
								8, PropModeReplace, 
								(unsigned char *) get_item_contents_by_pos(num), 
								(int) strlen(get_item_contents_by_pos(num)));
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
			if (get_item_contents_by_pos(pos) != NULL)
			{
				XChangeProperty(g_disp, reqwin, cbhm_atoms[i], atomUTF8String, 
								8, PropModeReplace, 
								(unsigned char *) get_item_contents_by_pos(pos) , 
								(int) strlen(get_item_contents_by_pos(pos)));
			}
			pos--;
			if (pos < 0)
				pos = HISTORY_QUEUE_NUMBER-1;
		}
	}
	else if (strcmp("get raw", ev->data.b) == 0)
	{
		if (get_storage_start_addr != NULL)
		{
			XChangeProperty(g_disp, reqwin, atomCBHM_cRAW, XA_STRING, 
							8, PropModeReplace, 
							(unsigned char *) get_storage_start_addr(),
							(int) get_total_storage_size());
		}
	}
	else if (strcmp("show", ev->data.b) == 0)
	{
		clipdrawer_activate_view(ad);
	}

	XFlush(g_disp);

	return TRUE;
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
//	elm_selection_set(1, obj, /*mark up*/1, p);
	Ecore_X_Window setwin = get_selection_secondary_target_win();
	if (setwin == None)
		return 0;

	if (sdata == NULL)
		return 0;

	int slen = strlen(sdata);

	fprintf(stderr, "## cbhm xwin = 0x%x, d = %s\n", setwin, sdata);

	ecore_x_selection_secondary_set(setwin, sdata, slen);
}
