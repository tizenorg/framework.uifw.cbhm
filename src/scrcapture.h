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




#ifndef _SCRCAPTURE_H_
#define _SCRCAPTURE_H_

#include <Elementary.h>

struct _SCaptureData {
	int svi_handle;

	Eina_Bool svi_init:1;
};

#include "cbhm.h"

SCaptureData *init_screencapture(AppData *ad);
void depose_screencapture(SCaptureData *sd);
void capture_current_screen(AppData *ad);
#endif // _SCRCAPTURE_H_
