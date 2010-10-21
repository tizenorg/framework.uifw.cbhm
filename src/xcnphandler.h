#ifndef _xcnphandler_h_
#define _xcnphandler_h_

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>

#include <Ecore_X.h>
#include <utilX.h>

int xcnp_init(void *data);
int xcnp_shutdown();

static int _init_atoms();
static void _set_cbhmwin_prop();

int increment_current_history_position();
int get_current_history_position();
int add_to_storage_buffer(void *data, char *src, int len);
int print_storage_buffer();

int send_convert_selection();
int set_clipboard_manager_owner();
int set_selection_owner();
int get_selection_content(void *data);
int processing_selection_request(Ecore_X_Event_Selection_Request *ev);

static int _cbhm_init();
static void _cbhm_fini();
static int _xsel_clear_cb(void *data, int ev_type, void *event);
static int _xsel_request_cb(void *data, int ev_type, void *event);
static int _xsel_notify_cb(void *data, int ev_type, void *event);
static int _xclient_msg_cb(void *data, int ev_type, void *event);
static int _xfocus_out_cb(void *data, int ev_type, void *event);
static Ecore_X_Window get_selection_secondary_target_win();
int set_selection_secondary_data(char *sdata);
	
#define ATOM_CLIPBOARD_NAME "CLIPBOARD"
#define ATOM_CLIPBOARD_MANAGER_NAME "CLIPBOARD_MANAGER"
#define ATOM_CBHM_OUTBUF "CBHM_BUF"
#define CLIPBOARD_MANAGER_WINDOW_TITLE_STRING "X11_CLIPBOARD_HISTORY_MANAGER"

static Ecore_X_Display* g_disp = NULL;
static Ecore_X_Window g_rootwin = None;
static Ecore_X_Window g_evtwin = None;

/* all atoms are global variables */
static Atom atomPrimary;
static Atom atomSecondary;
//static Atom atomTarget;
static Atom atomClipboard;
static Atom atomCBHM;
static Atom atomCBOut;
static Atom atomInc;
static Atom atomTargets;
static Atom atomUTF8String;
static Atom atomHtmltext;

#endif //_xcnphandler_h_

