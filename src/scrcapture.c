#include "common.h"
#include "cbhm_main.h"
#include "xcnphandler.h"
#include "scrcapture.h"
#include "clipdrawer.h"

#include <sys/ipc.h>
#include <sys/shm.h>

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <utilX.h>

#define IMAGE_SAVE_DIR "/opt/media/Images and videos/My photo clips"
#define IMAGE_SAVE_FILE_TYPE ".jpg"
#define CAPTURE_SOUND_FILE "/usr/share/cbhm/sounds/Shutter_01.wav"
#define CAPTURE_SOUND_TIMEOUT_SEC 2
		        
#include <mmf/mm_sound_private.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

static pthread_mutex_t g_sound_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_sound_cond = PTHREAD_COND_INITIALIZER;
static Eina_Bool g_shot = EINA_FALSE;

typedef struct tag_captureimginfo
{
	char filename[256];
	Evas_Object *eo;
	char *imgdata;
} captureimginfo_t;

static Eina_Bool get_image_filename_with_date(char *dstr)
{
	time_t tim = time(NULL);
	struct tm *now = localtime(&tim);
	struct timeval tv;
	gettimeofday(&tv, NULL);
	sprintf(dstr, "%s/screen-%d%02d%02d%02d%02d%02d%0ld%s", 
			IMAGE_SAVE_DIR,
			now->tm_year+1900, now->tm_mon+1, now->tm_mday,
			now->tm_hour, now->tm_min,
			now->tm_sec, tv.tv_usec,
			IMAGE_SAVE_FILE_TYPE);
	return EINA_TRUE;
}

static void _sound_callback(void *data)
{
		DTRACE("_sound_callback\n");
        pthread_cond_broadcast(&g_sound_cond);
        return;
}

static void _play_capture_sound()
{
	int ret, step, handle;
	MMSoundParamType pparam = {0,};
	Eina_Bool sync = EINA_FALSE;
	struct timespec timeout;
	struct timeval tv;
	ret = mm_sound_set_path(MM_SOUND_GAIN_CAMERA, MM_SOUND_PATH_SPK, MM_SOUND_PATH_NONE, MM_SOUND_PATH_OPTION_NONE);	 
	if (ret != MM_ERROR_NONE)
	{
		DTRACE("mm_sound_set_path is failed\n");
		return;
	}
	ret = mm_sound_volume_get_step(VOLUME_TYPE_FIXED, &step);
	if (ret != MM_ERROR_NONE)
	{
		DTRACE("mm_sound_volume_get_step is failed\n");
		return;
	}
	if (pthread_mutex_trylock(&g_sound_lock) == EBUSY)
	{
		DTRACE("trylock is fail - g_sound_lock\n");
		return;
	}
	pparam.filename = CAPTURE_SOUND_FILE;
	pparam.loop = 1;
	pparam.volume = step-1;
	pparam.callback = _sound_callback;

	if (mm_sound_play_loud_solo_sound(CAPTURE_SOUND_FILE, 
									  VOLUME_TYPE_FIXED, _sound_callback, 
									  NULL, &handle) 
		== MM_ERROR_NONE)
	{
		if(sync)
		{
			gettimeofday(&tv, NULL);
			timeout.tv_sec = tv.tv_sec + CAPTURE_SOUND_TIMEOUT_SEC;
			timeout.tv_nsec = tv.tv_usec;
			if(ETIMEDOUT == pthread_cond_timedwait(&g_sound_cond, &g_sound_lock, &timeout))
			{
				if(handle>0)
					mm_sound_stop_sound(handle);
			}
		}
	}
	else
	{
		DTRACE("effect sound play failed\n");
		pthread_mutex_unlock(&g_sound_lock);
		return;
	}
	pthread_mutex_unlock(&g_sound_lock);
	DTRACE("sound play success\n");
}

static Eina_Bool _scrcapture_capture_postprocess(void* data)
{
	captureimginfo_t *capimginfo = data;

	DTIME("start capture postprocess - %s\n", capimginfo->filename);

	if (!evas_object_image_save(capimginfo->eo, capimginfo->filename, NULL, NULL)) 
	{
		DTRACE("screen capture save fail\n");
		return EINA_FALSE;
	}

	DTIME("end capture postprocess - %s\n", capimginfo->filename);

	char *imgpath = NULL;
	imgpath = malloc(strlen(capimginfo->filename)+strlen("file://")+2);
	snprintf(imgpath, strlen(capimginfo->filename)+strlen("file://")+1,
			 "%s%s", "file://", capimginfo->filename);
	DTRACE("add to image history = %s\n", imgpath+strlen("file://"));
	clipdrawer_add_item(imgpath+strlen("file://"), GI_IMAGE);
	free(imgpath);
        
	evas_object_del(capimginfo->eo);
	free(capimginfo->imgdata);
	free(capimginfo);

	_play_capture_sound();

	g_shot = EINA_FALSE;

	return EINA_FALSE;
}

Eina_Bool capture_current_screen(void *data)
{
	struct appdata *ad = data;

	if (g_shot)
	{
		DTRACE("too early to capture current screen\n");
		return EINA_FALSE;
	}
	g_shot = EINA_TRUE;

	captureimginfo_t *capimginfo = NULL;
	capimginfo = malloc(sizeof(captureimginfo_t) * 1);
	get_image_filename_with_date(capimginfo->filename);
	DTRACE("capture current screen\n");

	int width, height;
	width = ad->root_w;
	height = ad->root_h;
	capimginfo->imgdata = malloc(sizeof(char) * (width*height*4) + 1);
	capimginfo->eo = evas_object_image_add(ad->evas);

	char *scrimage = NULL;
	scrimage = scrcapture_capture_screen_by_xv_ext(width, height);
	if (scrimage)
		memcpy(capimginfo->imgdata, scrimage, width*height*4);
	scrcapture_release_screen_by_xv_ext(scrimage);

	if (scrimage == NULL || capimginfo->eo == NULL || capimginfo->imgdata == NULL) 
	{
		DTRACE("screen capture fail\n");
		free(capimginfo->imgdata);
		if (capimginfo->eo)
			evas_object_del(capimginfo->eo);
		free(capimginfo);
		return EINA_FALSE;
	}

	DTRACE("screen capture prepared\n");

	evas_object_image_data_set(capimginfo->eo, NULL);
	evas_object_image_size_set(capimginfo->eo, width, height);
	evas_object_image_data_set(capimginfo->eo, capimginfo->imgdata);
	evas_object_image_data_update_add(capimginfo->eo, 0, 0, width, height);
	evas_object_resize(capimginfo->eo, width, height);

	ecore_idler_add(_scrcapture_capture_postprocess, capimginfo);

	return EINA_TRUE;
}

static Eina_Bool scrcapture_keydown_cb(void *data, int type, void *event)
{
	struct appdata *ad = data;
	Ecore_Event_Key *ev = event;

	if (!strcmp(ev->keyname, KEY_END))
		clipdrawer_lower_view(ad);

	return ECORE_CALLBACK_PASS_ON;
}

int init_scrcapture(void *data)
{
	struct appdata *ad = data;

	int result = 0;

	/* Key Grab */
	Ecore_X_Display *xdisp = ecore_x_display_get();
	Ecore_X_Window xwin = (Ecore_X_Window)ecore_evas_window_get(ecore_evas_ecore_evas_get(ad->evas));

	ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, scrcapture_keydown_cb, ad);

	pthread_mutex_init(&g_sound_lock, NULL);
	pthread_cond_init(&g_sound_cond, NULL);

	return 0;
}

void close_scrcapture(void *data)
{
	struct appdata *ad = data;

	Ecore_X_Display *xdisp = ecore_x_display_get();
	Ecore_X_Window xwin = (Ecore_X_Window)ecore_evas_window_get(ecore_evas_ecore_evas_get(ad->evas));

	pthread_mutex_destroy(&g_sound_lock);
	pthread_cond_destroy(&g_sound_cond);
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
	if (!XGetWindowAttributes(get_display(), id, &attr)) 
	{
		return -1;
	}

	if (attr.map_state == IsViewable && attr.class == InputOutput) 
	{
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

	DTRACE("XQeuryTree\n");

	if (!XQueryTree(get_display(), id, &root, &parent, &children, &num)) 
	{
		return 0;
	}

	if (children) 
	{
		DTRACE("XFree\n");
		XFree(children);
	}

	return parent;
}

static Window find_capture_available_window( Window id, Visual** visual, int* depth, int* width, int* height) 
{
	XWindowAttributes attr;
	Window parent = id;
	Window orig_id = id;
	
	if (id == 0) 
	{
		return (Window) -1;
	}

	do 
	{
		id = parent;	
		DTRACE("find_capture - XGetWindowAttributes\n");

		if (!XGetWindowAttributes(get_display(), id, &attr)) 
		{
			return (Window) -1;
		}
		
		parent = _get_parent_window( id );

		if (attr.map_state == IsViewable
			       	&& attr.override_redirect == True
			       	&& attr.class == InputOutput && parent == attr.root )
       	{
			*depth = attr.depth;
			*width = attr.width;
			*height = attr.height;
			*visual = attr.visual;
			return id;
		}
	} while( parent != attr.root && parent != 0 ); //Failed finding a redirected window 
	

	DTRACE( "find_capture - cannot find id\n");
	XGetWindowAttributes (get_display(), orig_id, &attr);
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
		DTRACE("Window : 0x%lX\n", id);
		if (get_window_attribute(id, &depth, &visual, &width, &height) < 0) 
		{
			DTRACE("Failed to get the attributes from 0x%x\n", (unsigned int)id);
			return NULL;
		}
	}

	DTRACE("WxH : %dx%d\n", width, height);
	DTRACE("Depth : %d\n", depth >> 3);

	// NOTE: just add one more depth....
	si.shmid = shmget(IPC_PRIVATE, width * height * ((depth >> 3)+1), IPC_CREAT | 0666);
	if (si.shmid < 0) 
	{
		DTRACE("error at shmget\n");
		return NULL;
	}
	si.readOnly = False;
	si.shmaddr = shmat(si.shmid, NULL, 0);
	if (si.shmaddr == (char*)-1) 
	{
		shmdt(si.shmaddr);
		shmctl(si.shmid, IPC_RMID, 0);
		DTRACE("can't get shmat\n");
		return NULL;
	}

/*
	if (!need_redirecting) 
	{
		Window border;
		if (get_border_window(id, &border) < 0) 
		{
			need_redirecting = 1;
			printf("Failed to find a border, forcely do redirecting\n");
		} 
		else 
		{
			id = border;
			printf("Border window is found, use it : 0x%X\n", (unsigned int)id);
		}
	}

	if (need_redirecting) 
	{
		printf("XCompositeRedirectWindow");
		XCompositeRedirectWindow(get_display(), id, CompositeRedirectManual);
	}
*/


	DTRACE("XShmCreateImage\n");
	xim = XShmCreateImage(get_display(), visual, depth, ZPixmap, NULL, &si, width, height);
	if (!xim) 
	{
		shmdt(si.shmaddr);
		shmctl(si.shmid, IPC_RMID, 0);

/*
		if (need_redirecting) 
		{
			printf("XCompositeUnredirectWindow");
			XCompositeUnredirectWindow(get_display(), id, CompositeRedirectManual);
		}
*/
		return NULL;
	}

	*size = xim->bytes_per_line * xim->height;
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

	//sleep(1);
	// We can optimize this!
	captured_image = calloc(1, *size);
	if (captured_image) 
	{
		memcpy(captured_image, xim->data, *size);
	} 
	else 
	{
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

	xim = XGetImage(get_display(), xid, 0, 0,
					 width, height, AllPlanes, ZPixmap);

	*size = xim->bytes_per_line * xim->height;

	captured_image = calloc(1, *size);
	if (captured_image) 
	{
		memcpy(captured_image, xim->data, *size);
	} 
	else 
	{
		DTRACE("calloc error");
	}

	return captured_image;
}

char *scrcapture_capture_screen_by_xv_ext(int w, int h)
{
	return createScreenShot(w, h);
}

void scrcapture_release_screen_by_xv_ext(const char *s)
{
	releaseScreenShot(s);
}
