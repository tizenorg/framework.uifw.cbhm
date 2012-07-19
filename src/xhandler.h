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

#ifndef _XCNPHANDLER_H_
#define _XCNPHANDLER_H_

#include <Ecore_X.h>
#include <Ecore.h>

struct _XHandlerData {
	Ecore_Event_Handler *xsel_clear_handler;
	Ecore_Event_Handler *xsel_request_handler;
	Ecore_Event_Handler *xsel_notify_handler;
	Ecore_Event_Handler *xclient_msg_handler;
	Ecore_Event_Handler *xfocus_out_handler;
	Ecore_Event_Handler *xproperty_notify_handler;
	Ecore_Event_Handler *xwindow_destroy_handler;

	Ecore_X_Atom atomInc;
	Ecore_X_Atom atomWindowRotate;

	Ecore_X_Atom atomCBHM_MSG;
	Ecore_X_Atom atomCBHM_ITEM;
	Ecore_X_Atom atomXKey_MSG;

	Ecore_X_Atom atomUTF8String;
	Ecore_X_Atom atomCBHMCount;

	Ecore_Timer *selection_timer;
};

#include "cbhm.h"
#include "item_manager.h"
#include "xconverter.h"

XHandlerData *init_xhandler(AppData *data);
void depose_xhandler(XHandlerData *xd);
Eina_Bool set_selection_owner(AppData *ad, Ecore_X_Selection selection, CNP_ITEM *item);
void slot_property_set(AppData *ad, int index);
void slot_item_count_set(AppData *ad);

#define SELECTION_CHECK_TIME 15.0

#endif
