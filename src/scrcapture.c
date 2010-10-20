#include "common.h"
#include "cbhm_main.h"
#include "xcnphandler.h"
#include "scrcapture.h"

#include <sys/ipc.h>
#include <sys/shm.h>

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <utilX.h>

#include <stdio.h>

static Eina_Bool capture_current_screen(void *data)
{
//	char *captured_image = scrcapture_capture_screen_by_xv_ext(480, 800);


//	scrcapture_release_screen_by_xv_ext(captured_image);

	return EINA_TRUE;
}

static Eina_Bool scrcapture_keydown_cb(void *data, int type, void *event)
{
	Ecore_Event_Key *ev = event;

	/* FIXME : it will be changed to camera+select, not ony one key */

	if(!strcmp(ev->keyname, KEY_CAMERA) || !strcmp(ev->keyname, KEY_SELECT))
	{
		DTRACE("keydown = %s\n", ev->keyname);
	}

	return ECORE_CALLBACK_PASS_ON;
}

int init_scrcapture(void *data)
{
	struct appdata *ad = data;

	int result = 0;

	/* Key Grab */
	Ecore_X_Display *xdisp = ecore_x_display_get();
	Ecore_X_Window xwin = (Ecore_X_Window)ecore_evas_window_get(ecore_evas_ecore_evas_get(ad->evas));

	result = utilx_grab_key(xdisp, xwin, KEY_SELECT, SHARED_GRAB);
	if(!!result)
		DTRACE("KEY_HOME key grab is failed\n");

//	result = utilx_grab_key(xdisp, xwin, KEY_CAMERA, SHARED_GRAB);
//	if(!result)
//		DTRACE( "KEY_CAMERA key grab\n");

	ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, scrcapture_keydown_cb, NULL);

	return 0;
}

void close_scrcapture(void *data)
{
	struct appdata *ad = data;

	Ecore_X_Display *xdisp = ecore_x_display_get();
	Ecore_X_Window xwin = (Ecore_X_Window)ecore_evas_window_get(ecore_evas_ecore_evas_get(ad->evas));

	utilx_ungrab_key(xdisp, xwin, KEY_SELECT);
//	utilx_ungrab_key(xdisp, xwin, KEY_CAMERA);
}


inline static Ecore_X_Display *get_display(void)
{
	return ecore_x_display_get();
}

static int get_window_attribute(Window id, int *depth, Visual **visual, int *width, int *height)
{
//	assert(id);
	XWindowAttributes attr;

	DTRACE("XGetWindowAttributes\n");
	if (!XGetWindowAttributes(get_display(), id, &attr)) {
		return -1;
	}

	if (attr.map_state == IsViewable && attr.class == InputOutput) {
		*depth = attr.depth;
		*width = attr.width;
		*height= attr.height;
		*visual= attr.visual;
	}

	return 0;
}

static Window _get_parent_window( Window id )
{
	Window root;
	Window parent;
	Window* children;
	unsigned int num;

	DTRACE( "XQeuryTree\n");

	if (!XQueryTree(get_display(), id, &root, &parent, &children, &num)) 
	{
		return 0;
	}

	if( children ) {
		DTRACE( "XFree\n");
		XFree(children);
	}

	return parent;
}

static Window find_capture_available_window( Window id, Visual** visual, int* depth, int* width, int* height ) 
{
	XWindowAttributes attr;
	Window parent = id;
	Window orig_id = id;
	
	if( id == 0 ) {
		return (Window) -1;
	}

	do {
		id = parent;	
		DTRACE( "## find_capture - XGetWindowAttributes\n");

		if (!XGetWindowAttributes(get_display(), id, &attr)) 
		{
			return (Window) -1;
		}
		
		parent = _get_parent_window( id );

		if( attr.map_state == IsViewable
			       	&& attr.override_redirect == True
			       	&&  attr.class == InputOutput && parent == attr.root )
	       	{
			*depth = attr.depth;
			*width = attr.width;
			*height = attr.height;
			*visual = attr.visual;
			return id;
		}
	} while( parent != attr.root && parent != 0 ); //Failed finding a redirected window 
	

	DTRACE( "## find_capture - cannot find id\n");
	XGetWindowAttributes( get_display(), orig_id, &attr );
	*depth = attr.depth;
	*width = attr.width;
	*height = attr.height;
	*visual = attr.visual;
		
	return (Window) 0;
	
}

char *scrcapture_screen_capture(Window oid, int *size)
{
	XImage *xim;
	XShmSegmentInfo si;
	Pixmap pix;
	int depth;
	int width;
	int height;
	Visual *visual;
	char *captured_image;
	Window id;
	
	id = find_capture_available_window(ecore_x_window_focus_get(), &visual, &depth, &width, &height);

	if (id == 0 || id == -1 || id == oid)
	{
		DTRACE( "Window : 0x%lX\n", id);
		if (get_window_attribute(id, &depth, &visual, &width, &height) < 0) 
		{
			DTRACE( "Failed to get the attributes from 0x%x\n", (unsigned int)id);
			return NULL;
		}
	}

	DTRACE( "WxH : %dx%d\n", width, height);
	DTRACE( "Depth : %d\n", depth >> 3);

	// NOTE: just add one more depth....
	si.shmid = shmget(IPC_PRIVATE, width * height * ((depth >> 3)+1), IPC_CREAT | 0666);
	if (si.shmid < 0) {
		DTRACE( "## error at shmget\n");
		return NULL;
	}
	si.readOnly = False;
	si.shmaddr = shmat(si.shmid, NULL, 0);
	if (si.shmaddr == (char*)-1) {
		shmdt(si.shmaddr);
		shmctl(si.shmid, IPC_RMID, 0);
		DTRACE( "## can't get shmat\n");
		return NULL;
	}

/*
	if (!need_redirecting) {
		Window border;
		if (get_border_window(id, &border) < 0) {
			need_redirecting = 1;
			printf("Failed to find a border, forcely do redirecting\n");
		} else {
			id = border;
			printf("Border window is found, use it : 0x%X\n", (unsigned int)id);
		}
	}

	if (need_redirecting) {
		printf("XCompositeRedirectWindow");
		XCompositeRedirectWindow(get_display(), id, CompositeRedirectManual);
	}
*/

	DTRACE( "XShmCreateImage\n");
	xim = XShmCreateImage(get_display(), visual, depth, ZPixmap, NULL, &si, width, height);
	if (!xim) {
		shmdt(si.shmaddr);
		shmctl(si.shmid, IPC_RMID, 0);

/*
		if (need_redirecting) {
			printf("XCompositeUnredirectWindow");
			XCompositeUnredirectWindow(get_display(), id, CompositeRedirectManual);
		}
*/
		return NULL;
	}

	*size = xim->bytes_per_line * xim->height;
	DTRACE( "## size = %d\n", *size);
	xim->data = si.shmaddr;

	DTRACE("XCompositeNameWindowPixmap\n");
	pix = XCompositeNameWindowPixmap(get_display(), id);

	DTRACE("XShmAttach\n");
	XShmAttach(get_display(), &si);

	DTRACE("XShmGetImage\n");
	XShmGetImage(get_display(), pix, xim, 0, 0, 0xFFFFFFFF);

	//XUnmapWindow(disp, id);
	//XMapWindow(disp, id);
	DTRACE("XSync\n");
	XSync(get_display(), False);

		DTRACE( "## data dump - start\n");
		
		int i = 0;
		for (i = 0; i < (*size/1000); i++)
		{
			DTRACE( "%X", xim->data[i]);
			if ((i % 24) == 0)
				DTRACE( "\n");
		}

		DTRACE( "## data end - start\n");

	//sleep(1);
	// We can optimize this!
	captured_image = calloc(1, *size);
	if (captured_image) {
		memcpy(captured_image, xim->data, *size);
	} else {
		DTRACE("calloc error");
	}

	DTRACE("XShmDetach");
	XShmDetach(get_display(), &si);
	DTRACE("XFreePixmap\n");
	XFreePixmap(get_display(), pix);
	DTRACE("XDestroyImage\n");
	XDestroyImage(xim);

/*
	if (need_redirecting) {
		printf("XCompositeUnredirectWindow");
		XCompositeUnredirectWindow(get_display(), id, CompositeRedirectManual);
	}
*/

	shmdt(si.shmaddr);
	shmctl(si.shmid, IPC_RMID, 0);
	return captured_image;
}

char *scrcapture_capture_screen_by_x11(Window xid, int *size)
{
	XImage *xim;
	int depth;
	int width;
	int height;
	Visual *visual;
	char *captured_image;


	DTRACE("Window : 0x%lX\n", xid);
	if (get_window_attribute(xid, &depth, &visual, &width, &height) < 0) 
	{
		DTRACE("Failed to get the attributes from 0x%x\n", (unsigned int)xid);
		return NULL;
	}

	DTRACE("WxH : %dx%d\n", width, height);
	DTRACE("Depth : %d\n", depth >> 3);

	xim = XGetImage (get_display(), xid, 0, 0,
					 width, height, AllPlanes, ZPixmap);

	*size = xim->bytes_per_line * xim->height;

	captured_image = calloc(1, *size);
	if (captured_image) {
		memcpy(captured_image, xim->data, *size);
	} else {
		DTRACE("calloc error");
	}

	return captured_image;
}

char *scrcapture_capture_screen_by_xv_ext(int width, int height)
{
	char *captured_image;

	captured_image = createScreenShot(width, height);
}

void scrcapture_release_screen_by_xv_ext(const char *s)
{
	releaseScreenShot(s);
}
