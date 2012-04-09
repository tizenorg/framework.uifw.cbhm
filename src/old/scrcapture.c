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

//#define IMAGE_SAVE_DIR "/opt/media/Images and videos/My photo clips"
#define IMAGE_SAVE_DIR "/opt/media/Images"
#define IMAGE_SAVE_FILE_TYPE ".png"

#include <errno.h>
#include <sys/time.h>
#include <svi/svi.h>
#include <sys/types.h>

#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <pixman.h>

static int svi_handle = -1;
static Eina_Bool g_shot = EINA_FALSE;
static XImage *ximage;
static XShmSegmentInfo x_shm_info;
static char *rot_buffer;


typedef struct tag_captureimginfo
{
	char filename[256];
	Evas_Object *eo;
	char *imgdata;
} captureimginfo_t;

static const char *createScreenShotSW(Display *d, int width, int height);
void releaseScreenShotSW(Display *d, const char * ss);
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


static void _play_capture_sound()
{
	int ret = SVI_ERROR;

	if (svi_handle != -1)
		ret = svi_play(svi_handle, SVI_VIB_OPERATION_SHUTTER, SVI_SND_OPERATION_SCRCAPTURE);

	if (ret != SVI_SUCCESS)
	{
		DTRACE("play file failed\n");
	}
	else
	{
		DTRACE("play file success\n");
	}
}

static Eina_Bool hide_small_popup(void *data)
{
	struct appdata *ad = data;
	ad->popup_timer = NULL;
	evas_object_hide(ad->small_popup);
	return ECORE_CALLBACK_CANCEL;
}

static void show_small_popup(struct appdata *ad, char* msg)
{
	if (!ad->small_popup)
		ad->small_popup = elm_tickernoti_add(NULL);
	if (!ad->small_win)
		ad->small_win = elm_tickernoti_win_get(ad->small_popup);

	elm_object_style_set(ad->small_popup, "info");
	elm_tickernoti_label_set(ad->small_popup, msg);
	elm_tickernoti_orientation_set(ad->small_popup, ELM_TICKERNOTI_ORIENT_BOTTOM);
	evas_object_show(ad->small_popup);
	ad->popup_timer = ecore_timer_add(2, hide_small_popup, ad);
}

static void play_screen_capture_effect()
{
	struct appdata *ad = g_get_main_appdata();
	show_small_popup(ad, "Screen capture success");
	Ecore_X_Display *disp = ecore_x_display_get();
	Ecore_X_Window root_win = ecore_x_window_root_first_get();
	DTRACE("disp: 0x%x, root_win: 0x%x\n", disp, root_win);
	utilx_show_capture_effect(disp, root_win);
	_play_capture_sound();
}

static Eina_Bool _scrcapture_capture_postprocess(void* data)
{
	captureimginfo_t *capimginfo = data;

	DTIME("start capture postprocess - %s\n", capimginfo->filename);

	if (!evas_object_image_save(capimginfo->eo, capimginfo->filename, NULL, "compress=1"))
	{
		DTRACE("screen capture save fail\n");
		g_shot = EINA_FALSE;
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
	play_screen_capture_effect();
	DTIME("end current capture\n");

	g_shot = EINA_FALSE;

	return EINA_FALSE;
}

Eina_Bool capture_current_screen(void *data)
{
	struct appdata *ad = data;
	if (!utilx_get_screen_capture(XOpenDisplay(NULL)))
	{
		show_small_popup(ad, "Screen capture disabled");
		DTRACE("utilx_get_screen_capture: disable\n");
		return EINA_FALSE;
	}
	else
		DTRACE("utilx_get_screen_capture: enable\n");

	if (g_shot)
	{
		DTRACE("too early to capture current screen\n");
		return EINA_FALSE;
	}
	g_shot = EINA_TRUE;

	DTIME("start current capture\n");

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
	scrimage = createScreenShotSW(ecore_x_display_get(), width, height);
	if (scrimage)
		memcpy(capimginfo->imgdata, scrimage, width*height*4);
	releaseScreenShotSW(ecore_x_display_get(), scrimage);

	if (scrimage == NULL || capimginfo->eo == NULL || capimginfo->imgdata == NULL)
	{
		DTRACE("screen capture fail\n");
		free(capimginfo->imgdata);
		if (capimginfo->eo)
			evas_object_del(capimginfo->eo);
		free(capimginfo);
		g_shot = EINA_FALSE;
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
//	Ecore_X_Display *xdisp = ecore_x_display_get();
//	Ecore_X_Window xwin = (Ecore_X_Window)ecore_evas_window_get(ecore_evas_ecore_evas_get(ad->evas));

	ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, scrcapture_keydown_cb, ad);

	if (svi_init(&svi_handle) != SVI_SUCCESS)
	{
		DTRACE("svi init failed\n");
		svi_handle = -1;
		return -1;
	}

	return 0;
}

void close_scrcapture(void *data)
{
	struct appdata *ad = data;

//	Ecore_X_Display *xdisp = ecore_x_display_get();
//	Ecore_X_Window xwin = (Ecore_X_Window)ecore_evas_window_get(ecore_evas_ecore_evas_get(ad->evas));

	if (svi_handle != -1)
		svi_fini(svi_handle);
	if (ad->small_popup)
		evas_object_del(ad->small_popup);
	if (ad->small_win)
		evas_object_del(ad->small_win);
}

static int get_window_attribute(Window id, int *depth, Visual **visual, int *width, int *height)
{
//	assert(id);
	XWindowAttributes attr;

	DTRACE("XGetWindowAttributes\n");
	if (!XGetWindowAttributes(ecore_x_display_get(), id, &attr))
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

	if (!XQueryTree(ecore_x_display_get(), id, &root, &parent, &children, &num))
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

		if (!XGetWindowAttributes(ecore_x_display_get(), id, &attr))
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
	XGetWindowAttributes (ecore_x_display_get(), orig_id, &attr);
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
		XCompositeRedirectWindow(ecore_x_display_get(), id, CompositeRedirectManual);
	}
*/


	DTRACE("XShmCreateImage\n");
	xim = XShmCreateImage(ecore_x_display_get(), visual, depth, ZPixmap, NULL, &si, width, height);
	if (!xim)
	{
		shmdt(si.shmaddr);
		shmctl(si.shmid, IPC_RMID, 0);

/*
		if (need_redirecting)
		{
			printf("XCompositeUnredirectWindow");
			XCompositeUnredirectWindow(ecore_x_display_get(), id, CompositeRedirectManual);
		}
*/
		return NULL;
	}

	*size = xim->bytes_per_line * xim->height;
	xim->data = si.shmaddr;

	DTRACE("XCompositeNameWindowPixmap\n");
	pix = XCompositeNameWindowPixmap(ecore_x_display_get(), id);

	DTRACE("XShmAttach\n");
	XShmAttach(ecore_x_display_get(), &si);

	DTRACE("XShmGetImage\n");
	XShmGetImage(ecore_x_display_get(), pix, xim, 0, 0, 0xFFFFFFFF);

	//XUnmapWindow(disp, id);
	//XMapWindow(disp, id);
	DTRACE("XSync\n");
	XSync(ecore_x_display_get(), False);

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
	XShmDetach(ecore_x_display_get(), &si);
	DTRACE("XFreePixmap\n");
	XFreePixmap(ecore_x_display_get(), pix);
	DTRACE("XDestroyImage\n");
	XDestroyImage(xim);

/*
	if (need_redirecting) {
		printf("XCompositeUnredirectWindow");
		XCompositeUnredirectWindow(ecore_x_display_get(), id, CompositeRedirectManual);
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

	xim = XGetImage(ecore_x_display_get(), xid, 0, 0,
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

#define return_if_fail(cond)          {if (!(cond)) { printf ("%s : '%s' failed.\n", __FUNCTION__, #cond); return; }}
#define return_val_if_fail(cond, val) {if (!(cond)) { printf ("%s : '%s' failed.\n", __FUNCTION__, #cond); return val; }}
#define goto_if_fail(cond, dst)       {if (!(cond)) { printf ("%s : '%s' failed.\n", __FUNCTION__, #cond); goto dst; }}

int convert_image (uint32_t       *srcbuf,
               uint32_t       *dstbuf,
               pixman_format_code_t src_format,
               pixman_format_code_t dst_format,
               int src_width, int src_height,
               int dst_width, int dst_height,
               int             rotate)
{
	pixman_image_t *   src_img;
	pixman_image_t *   dst_img;
	pixman_transform_t transform;
	pixman_region16_t  clip;
	int                src_stride, dst_stride;
	int                src_bpp;
	int                dst_bpp;
	pixman_op_t        op;
	int                rotate_step;
	int                ret = False;

	return_val_if_fail (srcbuf != NULL, False);
	return_val_if_fail (dstbuf != NULL, False);
	return_val_if_fail (rotate <= 360 && rotate >= -360, False);

	op = PIXMAN_OP_SRC;

	src_bpp = PIXMAN_FORMAT_BPP (src_format) / 8;
	return_val_if_fail (src_bpp > 0, False);

	dst_bpp = PIXMAN_FORMAT_BPP (dst_format) / 8;
	return_val_if_fail (dst_bpp > 0, False);

	rotate_step = (rotate + 360) / 90 % 4;

	src_stride = src_width * src_bpp;
	dst_stride = dst_width * dst_bpp;

	src_img = pixman_image_create_bits (src_format, src_width, src_height, srcbuf, src_stride);
	dst_img = pixman_image_create_bits (dst_format, dst_width, dst_height, dstbuf, dst_stride);

	goto_if_fail (src_img != NULL, CANT_CONVERT);
	goto_if_fail (dst_img != NULL, CANT_CONVERT);

	pixman_transform_init_identity (&transform);

	if (rotate_step > 0)
	{
		int c, s, tx = 0, ty = 0;
		switch (rotate_step)
		{
		case 1:
			/* 270 degrees */
			c = 0;
			s = -pixman_fixed_1;
			ty = pixman_int_to_fixed (dst_width);
			break;
		case 2:
			/* 180 degrees */
			c = -pixman_fixed_1;
			s = 0;
			tx = pixman_int_to_fixed (dst_width);
			ty = pixman_int_to_fixed (dst_height);
			break;
		case 3:
			/* 90 degrees */
			c = 0;
			s = pixman_fixed_1;
			tx = pixman_int_to_fixed (dst_height);
			break;
		default:
			/* 0 degrees */
			c = 0;
			s = 0;
			break;
		}
		pixman_transform_rotate (&transform, NULL, c, s);
		pixman_transform_translate (&transform, NULL, tx, ty);
	}

	pixman_image_set_transform (src_img, &transform);

	pixman_image_composite (op, src_img, NULL, dst_img,
	                        0, 0, 0, 0, 0, 0, dst_width, dst_height);

	ret = True;

CANT_CONVERT:
	if (src_img)
		pixman_image_unref (src_img);
	if (dst_img)
		pixman_image_unref (dst_img);

	return ret;
}

int
getXwindowProperty (Display *d, Window          xwindow,
                    Atom            prop_atom,
                    Atom            type_atom,
                    unsigned char  *value,
                    int             nvalues)
{
	int    ret = 0;
	Atom    ret_type = None;
	int    ret_format;
	unsigned long  ret_nitems;
	unsigned long  ret_bytes_after;
	unsigned char *data = NULL;
	int    result;

	result = XGetWindowProperty (d,  xwindow,
	                             prop_atom, 0, 0x7fffffff, False, type_atom,
	                             &ret_type, &ret_format, &ret_nitems, &ret_bytes_after,
	                             &data);

	if (result != Success)
	{
		DTRACE("Getting a property is failed!");
		ret = 0;
	}
	else if (type_atom != ret_type)
		ret = 0;
	else if (ret_format != 32)
	{
		DTRACE("Format is not matched! (%d)", ret_format);
		ret = 0;
	}
	else if (ret_nitems == 0 || !data)
		ret = 0;
	else
	{
		if (ret_nitems < nvalues)
			nvalues = ret_nitems;

		memcpy (value, data, nvalues*sizeof(int));
		ret = nvalues;
	}

	if (data)
		XFree(data);

	return ret;
}

const char *createScreenShotSW(Display *d, int width, int height)
{
	Window  root;
	int     rotate;
	Atom    atom_rotaion;
	char   *ret = NULL;

	if (ximage)
	{
		XDestroyImage (ximage);
		ximage = NULL;
	}

	root  = RootWindow (d, DefaultScreen(d));

	ximage = XShmCreateImage (d, DefaultVisualOfScreen (DefaultScreenOfDisplay (d)), 24, ZPixmap, NULL,
	                          &x_shm_info, (unsigned int)width, (unsigned int)height);
	if (!ximage)
	{
		DTRACE("XShmCreateImage failed !\n");
		return NULL;
	}

	x_shm_info.shmid    = shmget (IPC_PRIVATE,
	                              ximage->bytes_per_line * ximage->height,
	                              IPC_CREAT | 0777);
	x_shm_info.shmaddr  = ximage->data = shmat (x_shm_info.shmid, 0, 0);
	x_shm_info.readOnly = False;

	if (!XShmAttach (d, &x_shm_info))
	{
		DTRACE("XShmAttach failed !\n");
		return NULL;
	}

	if (!XShmGetImage (d, root, ximage, 0, 0, AllPlanes))
	{
		DTRACE("XShmGetImage failed !\n");
		return NULL;
	}

	ret = ximage->data;

	atom_rotaion = XInternAtom (d, "X_SCREEN_ROTATION", True);
	if (!atom_rotaion ||
		!getXwindowProperty (d, root, atom_rotaion, XA_CARDINAL, (unsigned char*)&rotate, 1))
	{
		rotate = RR_Rotate_0;
	}

	if (rotate == RR_Rotate_90 || rotate == RR_Rotate_270)
	{
		rot_buffer = calloc (ximage->bytes_per_line * ximage->height, 1);

		convert_image ((uint32_t*)ximage->data,
                       (uint32_t*)rot_buffer,
                       PIXMAN_x8b8g8r8, PIXMAN_x8b8g8r8,
                       height, width, width, height,
                       (rotate == RR_Rotate_90) ? 90 : 270);

		ret = rot_buffer;
	}

	XSync (d, False);

	return ret;
}

void releaseScreenShotSW(Display *d, const char * ss)
{
	if (ximage)
	{
		XShmDetach (d, &x_shm_info);
		shmdt (x_shm_info.shmaddr);
		shmctl (x_shm_info.shmid, IPC_RMID, NULL);

		XDestroyImage (ximage);
		ximage = NULL;

		if (rot_buffer)
		{
			free (rot_buffer);
			rot_buffer = NULL;
		}
	}
}
