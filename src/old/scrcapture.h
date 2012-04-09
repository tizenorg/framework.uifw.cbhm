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
