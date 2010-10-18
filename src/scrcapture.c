#include "xcnphandler.h"
#include "scrcapture.h"

#include <sys/ipc.h>
#include <sys/shm.h>

#include <Ecore_X.h>
#include <utilX.h>

#include <stdio.h>

inline static Ecore_X_Display *get_display(void)
{
	return ecore_x_display_get();
}

static int get_window_attribute(Window id, int *depth, Visual **visual, int *width, int *height)
{
//	assert(id);
	XWindowAttributes attr;

	printf("XGetWindowAttributes");
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

	fprintf(stderr, "XQeuryTree\n");

	if (!XQueryTree(get_display(), id, &root, &parent, &children, &num)) 
	{
		return 0;
	}

	if( children ) {
		fprintf(stderr, "XFree\n");
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
		fprintf(stderr, "## find_capture - XGetWindowAttributes\n");

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
	

	fprintf(stderr, "## find_capture - cannot find id\n");
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
		fprintf(stderr, "Window : 0x%lX\n", id);
		if (get_window_attribute(id, &depth, &visual, &width, &height) < 0) 
		{
			fprintf(stderr, "Failed to get the attributes from 0x%x\n", (unsigned int)id);
			return NULL;
		}
	}

	fprintf(stderr, "WxH : %dx%d\n", width, height);
	fprintf(stderr, "Depth : %d\n", depth >> 3);

	// NOTE: just add one more depth....
	si.shmid = shmget(IPC_PRIVATE, width * height * ((depth >> 3)+1), IPC_CREAT | 0666);
	if (si.shmid < 0) {
		fprintf(stderr, "## error at shmget\n");
		return NULL;
	}
	si.readOnly = False;
	si.shmaddr = shmat(si.shmid, NULL, 0);
	if (si.shmaddr == (char*)-1) {
		shmdt(si.shmaddr);
		shmctl(si.shmid, IPC_RMID, 0);
		fprintf(stderr, "## can't get shmat\n");
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

	fprintf(stderr, "XShmCreateImage\n");
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
	fprintf(stderr, "## size = %d\n", *size);
	xim->data = si.shmaddr;

	fprintf(stderr,"XCompositeNameWindowPixmap\n");
	pix = XCompositeNameWindowPixmap(get_display(), id);

	fprintf(stderr,"XShmAttach\n");
	XShmAttach(get_display(), &si);

	fprintf(stderr,"XShmGetImage\n");
	XShmGetImage(get_display(), pix, xim, 0, 0, 0xFFFFFFFF);

	//XUnmapWindow(disp, id);
	//XMapWindow(disp, id);
	fprintf(stderr,"XSync\n");
	XSync(get_display(), False);

		fprintf(stderr, "## data dump - start\n");
		
		int i = 0;
		for (i = 0; i < (*size/1000); i++)
		{
			fprintf(stderr, "%X", xim->data[i]);
			if ((i % 24) == 0)
				fprintf(stderr, "\n");
		}

		fprintf(stderr, "## data end - start\n");

	//sleep(1);
	// We can optimize this!
	captured_image = calloc(1, *size);
	if (captured_image) {
		memcpy(captured_image, xim->data, *size);
	} else {
		perror("calloc");
	}

	fprintf(stderr,"XShmDetach");
	XShmDetach(get_display(), &si);
	fprintf(stderr,"XFreePixmap\n");
	XFreePixmap(get_display(), pix);
	fprintf(stderr,"XDestroyImage\n");
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


	fprintf(stderr, "Window : 0x%lX\n", xid);
	if (get_window_attribute(xid, &depth, &visual, &width, &height) < 0) 
	{
		fprintf(stderr, "Failed to get the attributes from 0x%x\n", (unsigned int)xid);
		return NULL;
	}

	fprintf(stderr, "WxH : %dx%d\n", width, height);
	fprintf(stderr, "Depth : %d\n", depth >> 3);

	xim = XGetImage (get_display(), xid, 0, 0,
					 width, height, AllPlanes, ZPixmap);

	*size = xim->bytes_per_line * xim->height;

	captured_image = calloc(1, *size);
	if (captured_image) {
		memcpy(captured_image, xim->data, *size);
	} else {
		perror("calloc");
	}

	return captured_image;
}

char *scrcapture_capture_screen_by_xv_ext(int width, int height)
{
	char *captured_image;

	captured_image = createScreenShot(480, 800);
}

void scrcapture_release_screen_by_xv_ext(const char *s)
{
	releaseScreenShot(s);
}
