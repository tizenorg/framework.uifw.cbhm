/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the License);
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an AS IS BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */




#include <utilX.h>
#include <sys/time.h>
#include <svi/svi.h>
#include <pixman.h>
#include <sys/ipc.h>
#include <Ecore_X.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrandr.h>

#ifdef USE_SYSPOPUP
#include <bundle.h>
#include <syspopup_caller.h>
#endif

#include "scrcapture.h"
#include "cbhm.h"
#include "item_manager.h"

#include <E_Notify.h>

//#define IMAGE_SAVE_DIR "/opt/media/Images and videos/My photo clips"
#define IMAGE_SAVE_DIR "/opt/media/Images"
#define IMAGE_SAVE_FILE_TYPE ".png"

#ifdef USE_SYSPOPUP
enum syspopup_type {
	SCREEN_CAPTURE_SUCCESS = 0,
	SCREEN_CAPTURE_DISABLED
};
#define SYSPOPUP_PARAM_LEN 3
#define SYSPOPUP_TYPE 0
#define CBHM_SYSPOPUP "cbhm_syspopup"
#else
#define SCREEN_CAPTURE_SUCCESS "Screen capture success"
#define SCREEN_CAPTURE_DISABLED "Screen capture disabled"
#endif

static char *createScreenShotSW(AppData *ad, int width, int height, XImage **ximage_ret, XShmSegmentInfo *x_shm_info);
static void releaseScreenShotSW(Ecore_X_Display *x_disp, char *ss, XImage *ximage, XShmSegmentInfo *x_shm_info);

SCaptureData *init_screencapture(AppData *ad)
{
	SCaptureData *sd = CALLOC(1, sizeof(SCaptureData));

	int ret;
	ret = svi_init(&sd->svi_handle);
	if(ret != SVI_SUCCESS)
	{
		DMSG("svi_init failed %d\n", ret);
		sd->svi_init = EINA_FALSE;
	}
	else
		sd->svi_init = EINA_TRUE;

	return sd;
}

void depose_screencapture(SCaptureData *sd)
{
	if (sd->svi_init)
		svi_fini(sd->svi_handle);
	FREE(sd);
	e_notification_shutdown();
}

static char *get_image_filename_with_date()
{
	time_t tim = time(NULL);
	struct tm *now = localtime(&tim);
	struct timeval tv;
	gettimeofday(&tv, NULL);

	size_t len = snprintf(NULL, 0, "%s/screen-%d%02d%02d%02d%02d%02d%0ld%s",
			IMAGE_SAVE_DIR,
			now->tm_year+1900, now->tm_mon+1, now->tm_mday,
			now->tm_hour, now->tm_min,
			now->tm_sec, tv.tv_usec,
			IMAGE_SAVE_FILE_TYPE) + 1;
	char *filepath = MALLOC(sizeof(char) * len);
	if (!filepath)
	{
		DTRACE("can't alloc filepath buffer\n");
		return NULL;
	}

	snprintf(filepath, len, "%s/screen-%d%02d%02d%02d%02d%02d%0ld%s",
			IMAGE_SAVE_DIR,
			now->tm_year+1900, now->tm_mon+1, now->tm_mday,
			now->tm_hour, now->tm_min,
			now->tm_sec, tv.tv_usec,
			IMAGE_SAVE_FILE_TYPE);
	return filepath;
}

#ifdef USE_SYSPOPUP
#define PRINT_CASE_WITH_FUNC(func, param) \
	case param: \
		DMSG(#func" fail: "#param"\n"); \
		break;

#define PRINT_ERROR(error) \
	print_bundle_error(__func__, error);

static void print_bundle_error(const char *func, int err)
{
	switch(errno)
	{
		PRINT_CASE_WITH_FUNC(func, EKEYREJECTED);
		PRINT_CASE_WITH_FUNC(func, EPERM);
		PRINT_CASE_WITH_FUNC(func, EINVAL);
		default:
			DMSG("unknown errno in %s: %d", func, err);
			break;
	}
}

#undef PRINT_ERROR
#undef PRINT_CASE_WITH_FUNC

static void launch_cbhm_syspopup(SCaptureData *sd, int type)
{
	char syspopup_type[SYSPOPUP_PARAM_LEN];

	bundle *b = bundle_create();
	if (!b)
	{
		DMSG("bundle_create() fail\n");
		return;
	}

	snprintf(syspopup_type, SYSPOPUP_PARAM_LEN, "%d", type);
	int ret = bundle_add(b, "CBHM_MSG_TYPE", syspopup_type);
	if (ret == -1)
	{
		PRINT_ERROR(errno);
	}
	else
	{
		ret = syspopup_launch(CBHM_SYSPOPUP, b);
		if (ret < 0)
			DMSG("syspopup_launch() fail: %d, %d\n", ret, errno);
	}
	ret = bundle_free(b);
	if (ret == -1)
		DMSG("bundle_free() fail: %d\n", errno);

	return;
}
#else
static void show_spopup(SCaptureData *sd, char *msg)
{
	E_Notification *n;
	e_notification_init();
	n = e_notification_new();
	e_notification_timeout_set(n, 2000);
	e_notification_summary_set(n, msg);
	e_notification_send(n, NULL, NULL);
}
#endif

static void play_scrcapture_effect(AppData *ad)
{
	SCaptureData *sd = ad->screencapture;

#ifdef USE_SYSPOPUP
	launch_cbhm_syspopup(sd, SCREEN_CAPTURE_SUCCESS);
#else
	show_spopup(sd, SCREEN_CAPTURE_SUCCESS);
	utilx_show_capture_effect(ad->x_disp, ad->x_root_win);
#endif

	int ret = 0;
	if (sd->svi_init)
	{
		ret = svi_play(sd->svi_handle, SVI_VIB_OPERATION_SHUTTER, SVI_SND_OPERATION_SCRCAPTURE);
		DMSG("svi_play return: %d\n", ret);
	}
}

void capture_current_screen(AppData *ad)
{
	SCaptureData *sd = ad->screencapture;
	ClipdrawerData *cd = ad->clipdrawer;
	Ecore_X_Display *disp = XOpenDisplay(NULL);

	DMSG("ad->x_disp: 0x%x, disp: 0x%x\n", ad->x_disp, disp);

	if (!utilx_get_screen_capture(disp))
	{
#ifdef USE_SYSPOPUP
		launch_cbhm_syspopup(sd, SCREEN_CAPTURE_DISABLED);
#else
		show_spopup(sd, SCREEN_CAPTURE_DISABLED);
#endif
		DMSG("utilx_get_screen_capture: disable\n");
		return;
	}

	DTIME("start current capture\n");

	int width, height;
	width = cd->root_w;
	height = cd->root_h;

	char *imgdata = MALLOC(sizeof(char) * (width*height*4) + 1);
	if (!imgdata)
	{
		DMSG("image buffer alloc fail\n");
		goto do_done;
	}

	Evas_Object *image = evas_object_image_add(cd->evas);
	if (!image)
	{
		DMSG("image add fail\n");
		goto do_done;
	}

	XImage *ximage;
	XShmSegmentInfo x_shm_info;
	char *scrimage = createScreenShotSW(ad, width, height, &ximage, &x_shm_info);
	if (!scrimage)
	{
		DMSG("screen capture fail\n");
		goto do_done;
	}

	memcpy(imgdata, scrimage, width*height*4);
	releaseScreenShotSW(ad->x_disp, scrimage, ximage, &x_shm_info);

	evas_object_image_data_set(image, NULL);
	evas_object_image_size_set(image, width, height);
	evas_object_image_data_set(image, imgdata);
	evas_object_image_data_update_add(image, 0, 0, width, height);
	evas_object_resize(image, width, height);
	char *filepath = get_image_filename_with_date();
	if (filepath)
	{
		if (!evas_object_image_save(image, filepath, NULL, "compress=1"))
		{
			DMSG("screen capture save fail\n");
			goto do_done;
		}
		item_add_by_data(ad, ad->targetAtoms[ATOM_INDEX_IMAGE].atom[0], filepath, strlen(filepath) + 1);

		play_scrcapture_effect(ad);
		DTIME("end current capture\n");
	}

do_done:
	if (image)
		evas_object_del(image);
	if (imgdata)
		FREE(imgdata);
	return;
}

#define return_if_fail(cond)          {if (!(cond)) { printf ("%s : '%s' failed.\n", __FUNCTION__, #cond); return; }}
#define return_val_if_fail(cond, val) {if (!(cond)) { printf ("%s : '%s' failed.\n", __FUNCTION__, #cond); return val; }}
#define goto_if_fail(cond, dst)       {if (!(cond)) { printf ("%s : '%s' failed.\n", __FUNCTION__, #cond); goto dst; }}

int convert_image(uint32_t *srcbuf, uint32_t *dstbuf,
               pixman_format_code_t src_format, pixman_format_code_t dst_format,
               int src_width, int src_height, int dst_width, int dst_height,
               int             rotate)
{
	pixman_image_t *src_img;
	pixman_image_t *dst_img;
	pixman_transform_t transform;
	pixman_region16_t clip;
	int src_stride, dst_stride;
	int src_bpp;
	int dst_bpp;
	pixman_op_t op;
	int rotate_step;
	int ret = False;

	return_val_if_fail (srcbuf != NULL, False);
	return_val_if_fail (dstbuf != NULL, False);
	return_val_if_fail (rotate <= 360 && rotate >= -360, False);

	op = PIXMAN_OP_SRC;

	src_bpp = PIXMAN_FORMAT_BPP(src_format) / 8;
	return_val_if_fail(src_bpp > 0, False);

	dst_bpp = PIXMAN_FORMAT_BPP(dst_format) / 8;
	return_val_if_fail(dst_bpp > 0, False);

	rotate_step = (rotate + 360) / 90 % 4;

	src_stride = src_width * src_bpp;
	dst_stride = dst_width * dst_bpp;

	src_img = pixman_image_create_bits(src_format, src_width, src_height, srcbuf, src_stride);
	dst_img = pixman_image_create_bits(dst_format, dst_width, dst_height, dstbuf, dst_stride);

	goto_if_fail (src_img != NULL, CANT_CONVERT);
	goto_if_fail (dst_img != NULL, CANT_CONVERT);

	pixman_transform_init_identity(&transform);

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
		pixman_transform_rotate(&transform, NULL, c, s);
		pixman_transform_translate(&transform, NULL, tx, ty);
	}

	pixman_image_set_transform(src_img, &transform);

	pixman_image_composite(op, src_img, NULL, dst_img,
	                        0, 0, 0, 0, 0, 0, dst_width, dst_height);

	ret = True;

CANT_CONVERT:
	if (src_img)
		pixman_image_unref(src_img);
	if (dst_img)
		pixman_image_unref(dst_img);

	return ret;
}

static char *createScreenShotSW(AppData *ad, int width, int height, XImage **ximage_ret, XShmSegmentInfo *x_shm_info)
{
	char *ret = NULL;

	XImage *ximage;

	ximage = XShmCreateImage(ad->x_disp, DefaultVisualOfScreen(DefaultScreenOfDisplay(ad->x_disp)), 24, ZPixmap, NULL,
	                          x_shm_info, (unsigned int)width, (unsigned int)height);
	if (!ximage)
	{
		DTRACE("XShmCreateImage failed !\n");
		return NULL;
	}

	x_shm_info->shmid = shmget(IPC_PRIVATE,
	                              ximage->bytes_per_line * ximage->height,
	                              IPC_CREAT | 0777);
	ximage->data = (void *)shmat(x_shm_info->shmid, NULL, 0);
	x_shm_info->shmaddr = ximage->data;
	x_shm_info->readOnly = False;

	if (!XShmAttach(ad->x_disp, x_shm_info))
	{
		DTRACE("XShmAttach failed !\n");
		releaseScreenShotSW(ad->x_disp, NULL, ximage, x_shm_info);
		return NULL;
	}

	if (!XShmGetImage(ad->x_disp, ad->x_root_win, ximage, 0, 0, AllPlanes))
	{
		DTRACE("XShmGetImage failed !\n");
		releaseScreenShotSW(ad->x_disp, NULL, ximage, x_shm_info);
		return NULL;
	}

	ret = ximage->data;

	Ecore_X_Atom atom_rotation = ecore_x_atom_get("X_SCREEN_ROTATION");

	int cnt;
	unsigned char *prop_data = NULL;
	int rotate = RR_Rotate_0;
	if (ecore_x_window_prop_property_get(ad->x_root_win, atom_rotation, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &cnt))
	{
		if (prop_data)
		{
			memcpy(&rotate, prop_data, sizeof(int));
			FREE(prop_data);
		}
	}

	if (rotate == RR_Rotate_90 || rotate == RR_Rotate_270)
	{
		char *rot_buffer;
		rot_buffer = CALLOC(ximage->bytes_per_line * ximage->height, 1);

		convert_image((uint32_t*)ximage->data,
                       (uint32_t*)rot_buffer,
                       PIXMAN_x8b8g8r8, PIXMAN_x8b8g8r8,
                       height, width, width, height,
                       (rotate == RR_Rotate_90) ? 90 : 270);

		ret = rot_buffer;
	}

	ecore_x_sync();

	*ximage_ret = ximage;
	return ret;
}

static void releaseScreenShotSW(Ecore_X_Display *x_disp, char *ss, XImage *ximage, XShmSegmentInfo *x_shm_info)
{
	if (ximage)
	{
		XShmDetach(x_disp, x_shm_info);
		shmdt(x_shm_info->shmaddr);
		shmctl(x_shm_info->shmid, IPC_RMID, NULL);

		if (ss != ximage->data)
			FREE(ss);

		XDestroyImage(ximage);
	}
}
