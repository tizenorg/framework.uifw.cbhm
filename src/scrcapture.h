/*
 * Copyright (c) 2009 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information"). You agree and
 * acknowledge that this software is owned by Samsung and you shall
 * not disclose such Confidential Information and shall use it only
 * in accordance with the terms of the license agreement you entered
 * into with SAMSUNG ELECTRONICS.  SAMSUNG make no representations
 * or warranties about the suitability of the software, either express
 * or implied, including but not limited to the implied warranties of
 * merchantability, fitness for a particular purpose, or non-infringement.
 * SAMSUNG shall not be liable for any damages suffered by
 * licensee arising out of or releated to this software.
 */

#ifndef _scrcapture_h_
#define _scrcapture_h_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>

// XV extension API - start 
const char* createScreenShot(int width, int height);
void releaseScreenShot(const char *ss);
// XV extension API - end

Eina_Bool capture_current_screen(void *data);
char *scrcapture_capture_screen_by_x11(Window xid, int *size);
char *scrcapture_capture_screen_by_xv_ext(int width, int height);
void scrcapture_release_screen_by_xv_ext(const char *s);

int init_scrcapture(void *data);
void close_scrcapture(void *data);

#endif // _scrcapture_h_
