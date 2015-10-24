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

#include <utilX.h>
#include "clipdrawer.h"
#include "item_manager.h"
#include "xconverter.h"

#define EDJ_PATH DATADIR"/edje"
#define APP_EDJ_FILE EDJ_PATH"/cbhmdrawer.edj"
#define GRP_MAIN "cbhmdrawer"

#define ANIM_DURATION 30 // 1 seconds
#define ANIM_FLOPS (0.5/30)
#define DEFAULT_WIDTH 720
#define BUFF (int)1024 //limited gengrid text buffer

#define EDJE_CLOSE_PART_PREFIX "background/title/close/bg"
#define EDJE_DELETE_MODE_PREFIX "background/title/delete/image"
#define EDJE_DELETE_ALL_BTN_PART_PREFIX "background/title/delete_all/image"

static Evas_Object *create_win(ClipdrawerData *cd, const char *name);
static Evas_Object *_grid_image_content_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_grid_combined_content_get(void *data, Evas_Object *obj, const char *part);
static char *_grid_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_grid_content_get(void *data, Evas_Object *obj, const char *part);
static void _grid_realized(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool clipdrawer_add_item(AppData *ad, CNP_ITEM *item);
static Eina_Bool clipdrawer_del_item(AppData *ad, CNP_ITEM *item);
static void _ok_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void _cancel_btn_cb(void *data, Evas_Object *obj, void *event_info);
static void _create_cbhm_popup(AppData *ad, ClipdrawerData *cd);
static void _show_cbhm_popup(ClipdrawerData *cd);
static void _hide_cbhm_popup(ClipdrawerData *cd);
static void clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _grid_item_button_clicked(void *data, Evas_Object *obj, void *event_info);
static void setting_win(Ecore_X_Display *x_disp, Ecore_X_Window x_root_win, Ecore_X_Window x_main_win);
static Ecore_X_Window isf_ise_window_get();
static void set_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win);
static void unset_transient_for(Ecore_X_Window x_main_win);
static void set_focus_for_app_window(Ecore_X_Window x_main_win, Eina_Bool enable);
static void rotate_cb(void *data, Evas_Object * obj, void *event);

static Evas_Event_Flags flick_end(void *data , void *event_info)
{
	AppData *ad = data;
	Elm_Gesture_Line_Info *event = event_info;
	int x_diff, y_diff;

	x_diff = event->momentum.x2 - event->momentum.x1;
	y_diff = event->momentum.y2 - event->momentum.y1;

	if (y_diff > 0 && y_diff > abs(x_diff))
		clipdrawer_lower_view(ad);

	return EVAS_EVENT_FLAG_NONE;
}

static void _change_gengrid_paste_textonly_mode(ClipdrawerData *cd)
{
	CNP_ITEM *item = NULL;

	Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);
	char *entry_text = NULL;

	while (gitem)
	{
		item = elm_object_item_data_get(gitem);
		if(!item)
			return;
		if (item->type_index == ATOM_INDEX_IMAGE)
		{
			if (cd->paste_text_only)
				elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
			else
				elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
		}

		if (item->gitem_style == GRID_ITEM_STYLE_COMBINED)
		{
			entry_text = string_for_entry_get(item->ad, item->type_index, item->data);
			entry_text = evas_textblock_text_markup_to_utf8(NULL, entry_text);

			if (cd->paste_text_only && entry_text && !strlen(entry_text))
				elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
			else
				elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
			if (entry_text) free(entry_text);
		}

		gitem = elm_gengrid_item_next_get(gitem);
	}
}

void clipdrawer_paste_textonly_set(AppData *ad, Eina_Bool textonly)
{
	ClipdrawerData *cd = ad->clipdrawer;
	if (cd->paste_text_only != textonly)
		cd->paste_text_only = textonly;
	DBG("paste textonly mode = %d", textonly);

	_change_gengrid_paste_textonly_mode(cd);
}

Eina_Bool clipdrawer_paste_textonly_get(AppData *ad)
{
	ClipdrawerData *cd = ad->clipdrawer;
	return cd->paste_text_only;
}

static Evas_Object *_load_edj(Evas_Object* win, const char *file, const char *group)
{
	Evas_Object *layout = elm_layout_add(win);
	if (!layout)
	{
		ERR("elm_layout_add return NULL");
		return NULL;
	}

	if (!elm_layout_file_set(layout, file, group))
	{
		ERR("elm_layout_file_set return FALSE");
		evas_object_del(layout);
		return NULL;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, layout);

	evas_object_show(layout);
	return layout;
}

static void _gengrid_select_cb(void *data, Evas_Object *obj, void *event)
{
	CNP_ITEM *item = data;
	AppData *ad = item->ad;
	ClipdrawerData *cd = ad->clipdrawer;

	elm_gengrid_item_selected_set(event, EINA_FALSE);
	if (cd->delbtn_clicked)
	{
		item_delete_by_CNP_ITEM(ad, item);
		cd->delbtn_clicked = EINA_FALSE;
	}
	else
	{
		if (delete_mode)
			return;

		if (cd->popup_activate)
			return;

		if (item->type_index != ATOM_INDEX_IMAGE || !cd->paste_text_only)
		{
			ad->clip_selected_item = item;
			if (is_cbhm_selection_owner(ad, ECORE_X_SELECTION_SECONDARY))
				cd->item_clicked = EINA_TRUE;
			else
				set_selection_owner(ad, ECORE_X_SELECTION_SECONDARY, NULL);
		}
	}
}

static Eina_Bool keydown_cb(void *data, int type, void *event)
{
	if (!data || !event) return ECORE_CALLBACK_DONE;
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;
	Ecore_Event_Key *ev = event;

	if (!SAFE_STRCMP(ev->keyname, "XF86Back"))
	{
		if (cd->popup_activate)
			_hide_cbhm_popup(ad->clipdrawer);
		else if (delete_mode)
			_delete_mode_set(ad, EINA_FALSE);
		else
			clipdrawer_lower_view(ad);
	}
	else if(!SAFE_STRCMP(ev->keyname, "XF86Home"))
	{
		clipdrawer_lower_view(ad);
		return ECORE_CALLBACK_PASS_ON;
	}

	return ECORE_CALLBACK_DONE;
}

// Access type callback for title close button
static char *
_title_close_btn_access_type_cb(void *data,
										  Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup("Button");
}

// Access information callback for title close button
static char *
_title_close_btn_access_info_cb(void *data,
										  Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup(S_CLOSE);
}

// Access type callback for title delete button
static char *
_title_delete_btn_access_type_cb(void *data,
											Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup("Button");
}


// Access information callback for title delete button
static char *
_title_delete_btn_access_info_cb(void *data,
											Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	if (delete_mode)
	  return strdup(S_DONE);
	else
	  return strdup(S_DELETE);
}

// Access type callback for title delete all button
static char *
_title_delete_all_btn_access_type_cb(void *data,
												 Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup("Button");
}

// Access information callback for title delete all button
static char *
_title_delete_all_btn_access_info_cb(void *data,
												 Evas_Object *obj)
{
	AppData *ad = (AppData *)data;
	if (!ad) return NULL;

	return strdup(S_DELETE_ALL);
}

// Access activate callback for title close button
static void
_title_close_btn_access_activate_cb(void *data,
												Evas_Object *obj,
												Elm_Object_Item *item)
{
	AppData *ad = (AppData *)data;
	if (!ad) return;

	clipdrawer_lower_view(ad);
}

// Access activate callback for title delete button
static void
_title_delete_btn_access_activate_cb(void *data,
												 Evas_Object *obj,
												 Elm_Object_Item *item)
{
	AppData *ad = (AppData *)data;
	if (!ad) return;

	_delete_mode_set(ad, !delete_mode);
}

// Access activate callback for title delete all button
static void
_title_delete_all_btn_access_activate_cb(void *data,
													  Evas_Object *obj,
													  Elm_Object_Item *item)
{
	AppData *ad = (AppData *)data;
	if (!ad) return;
	ClipdrawerData *cd = ad->clipdrawer;

	if ((item_count_get(ad, ATOM_INDEX_COUNT_ALL) - cd->locked_item_count) != 0)
		_show_cbhm_popup(ad->clipdrawer);
}

ClipdrawerData* init_clipdrawer(AppData *ad)
{
	ClipdrawerData *cd = calloc(1, sizeof(ClipdrawerData));
	const char *data;
	Evas_Object *part_obj, *access_obj;
	int rotations[4] = { 0, 90, 180, 270 };
	Evas_Coord w, h;

	/* create and setting window */
	if (!cd)
		return NULL;
	if (!(cd->main_win = create_win(cd, APPNAME)))
	{
		free(cd);
		return NULL;
	}
	if (!(cd->x_main_win = elm_win_xwindow_get(cd->main_win)))
	{
		free(cd);
		return NULL;
	}

	setting_win(ad->x_disp, ad->x_root_win, cd->x_main_win);

	/* edj setting */
	if (!(cd->main_layout = _load_edj(cd->main_win, APP_EDJ_FILE, GRP_MAIN)))
	{
		evas_object_del(cd->main_win);
		free(cd);
		return NULL;
	}

	elm_win_wm_rotation_available_rotations_set(cd->main_win, rotations, 4);
	evas_object_smart_callback_add(cd->main_win, "wm,rotation,changed", rotate_cb, ad);

	cd->popup_win = elm_win_add(NULL, "delete popup", ELM_WIN_MENU);
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	evas_object_resize(cd->popup_win, w, h);
	elm_win_alpha_set(cd->popup_win, EINA_TRUE);
	ecore_x_icccm_name_class_set(elm_win_xwindow_get(cd->popup_win),"APP_POPUP", "APP_POPUP");
	set_focus_for_app_window(elm_win_xwindow_get(cd->popup_win) , EINA_FALSE);
	set_transient_for(elm_win_xwindow_get(cd->popup_win), cd->x_main_win);
	elm_win_wm_rotation_available_rotations_set(cd->popup_win, rotations, 4);
	elm_win_conformant_set(cd->popup_win, EINA_TRUE);

	cd->popup_conform = elm_conformant_add(cd->popup_win);
	evas_object_size_hint_weight_set(cd->popup_conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(cd->popup_win, cd->popup_conform);

	_create_cbhm_popup(ad, cd);

	//double scale = elm_config_scale_get();
	Evas_Object* ly = elm_layout_edje_get(cd->main_layout);


	if (cd->root_w == WVGA_WIDTH && cd->root_h == WVGA_HEIGHT)
	{
		cd->height = WVGA_CLIPBOARD_H;
		cd->landscape_height = WVGA_CLIPBOARD_LANDSCAPE_H;
		cd->grid_item_bg_w = WVGA_GRID_ITEM_W;
		cd->grid_item_bg_h = WVGA_GRID_ITEM_H;
		cd->grid_image_item_w = WVGA_GRID_IMAGE_ITEM_W;
		cd->grid_image_item_h = WVGA_GRID_IMAGE_ITEM_H;
		cd->grid_combined_item_w = WVGA_GRID_COMBINED_ITEM_W;
		cd->grid_combined_item_h = WVGA_GRID_COMBINED_ITEM_H;
	}
	else
	{
		cd->height = CLIPBOARD_H;
		cd->landscape_height = CLIPBOARD_LANDSCAPE_H;
		cd->grid_item_bg_w = GRID_ITEM_W;
		cd->grid_item_bg_h = GRID_ITEM_H;
		cd->grid_image_item_w = GRID_IMAGE_ITEM_W;
		cd->grid_image_item_h = GRID_IMAGE_ITEM_H;
		cd->grid_combined_item_w = GRID_COMBINED_ITEM_W;
		cd->grid_combined_item_h = GRID_COMBINED_ITEM_H;

		cd->height = ELM_SCALE_SIZE(cd->height);
		cd->landscape_height = ELM_SCALE_SIZE(cd->landscape_height);
		cd->grid_item_bg_w = ELM_SCALE_SIZE(cd->grid_item_bg_w);
		cd->grid_item_bg_h = ELM_SCALE_SIZE(cd->grid_item_bg_h);
		cd->grid_image_item_w = ELM_SCALE_SIZE(cd->grid_image_item_w);
		cd->grid_image_item_h = ELM_SCALE_SIZE(cd->grid_image_item_h);
		cd->grid_combined_item_w = ELM_SCALE_SIZE(cd->grid_combined_item_w);
		cd->grid_combined_item_h = ELM_SCALE_SIZE(cd->grid_combined_item_h);
	}

	/* create and setting gengrid */
	elm_theme_extension_add(NULL, APP_EDJ_FILE);
	elm_object_signal_callback_add(cd->main_layout,
			"mouse,clicked,1", "*", clipdrawer_ly_clicked, ad);

	// Set accessibility function for title close button
	part_obj = (Evas_Object *)edje_object_part_object_get
		(elm_layout_edje_get(cd->main_layout), EDJE_CLOSE_PART_PREFIX);
	access_obj = elm_access_object_register(part_obj, cd->main_layout);

	elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE, _title_close_btn_access_type_cb, ad);
	elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO, _title_close_btn_access_info_cb, ad);
	elm_access_activate_cb_set(access_obj, _title_close_btn_access_activate_cb, ad);

	// Set access function for title delete button
	part_obj = (Evas_Object *)edje_object_part_object_get
		(elm_layout_edje_get(cd->main_layout), EDJE_DELETE_MODE_PREFIX);
	access_obj = elm_access_object_register(part_obj, cd->main_layout);

	elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE, _title_delete_btn_access_type_cb, ad);
	elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO, _title_delete_btn_access_info_cb, ad);
	elm_access_activate_cb_set(access_obj, _title_delete_btn_access_activate_cb, ad);

	// Set access function for title delete all button
	part_obj = (Evas_Object *)edje_object_part_object_get
		(elm_layout_edje_get(cd->main_layout), EDJE_DELETE_ALL_BTN_PART_PREFIX);
	access_obj = elm_access_object_register(part_obj, cd->main_layout);

	elm_access_info_cb_set(access_obj, ELM_ACCESS_TYPE, _title_delete_all_btn_access_type_cb, ad);
	elm_access_info_cb_set(access_obj, ELM_ACCESS_INFO, _title_delete_all_btn_access_info_cb, ad);
	elm_access_activate_cb_set(access_obj, _title_delete_all_btn_access_activate_cb, ad);

	cd->gengrid = elm_gengrid_add(cd->main_win);
	elm_object_style_set(cd->gengrid, "cbhm");
	elm_object_part_content_set(cd->main_layout, "historyitems", cd->gengrid);
	elm_gengrid_item_size_set(cd->gengrid, cd->grid_item_bg_w, cd->grid_item_bg_h);
	elm_gengrid_align_set(cd->gengrid, 0.0, 0.0);
	elm_gengrid_horizontal_set(cd->gengrid, EINA_TRUE);
//	elm_gengrid_bounce_set(cd->gengrid, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(cd->gengrid, EINA_FALSE);
//	evas_object_smart_callback_add(cd->gengrid, "selected", _grid_click_paste, ad);
	evas_object_size_hint_weight_set(cd->gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_gengrid_clear(cd->gengrid);

	evas_object_smart_callback_add(cd->gengrid, "realized", _grid_realized, NULL);

	cd->gic_image.item_style = "clipboard/image_style";
	cd->gic_image.func.text_get = NULL;
	cd->gic_image.func.content_get = _grid_image_content_get;
	cd->gic_image.func.state_get = NULL;
	cd->gic_image.func.del = NULL;

	cd->gic_text.item_style = "clipboard/text_style";
	cd->gic_text.func.text_get = _grid_text_get;
	cd->gic_text.func.content_get = _grid_content_get;
	cd->gic_text.func.state_get = NULL;
	cd->gic_text.func.del = NULL;

	cd->gic_combined.item_style = "clipboard/combined_style";
	cd->gic_combined.func.text_get = _grid_text_get;
	cd->gic_combined.func.content_get = _grid_combined_content_get;
	cd->gic_combined.func.state_get = NULL;
	cd->gic_combined.func.del = NULL;

	evas_object_show(cd->gengrid);

	ad->draw_item_add = clipdrawer_add_item;
	ad->draw_item_del = clipdrawer_del_item;
//	ad->x_main_win = cd->x_main_win;

	cd->keydown_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, keydown_cb, ad);
	cd->evas = evas_object_evas_get(cd->main_win);

	delete_mode = EINA_FALSE;
	cd->popup_activate = EINA_FALSE;
	cd->item_clicked = EINA_FALSE;
	cd->delbtn_clicked = EINA_FALSE;

	cd->event_rect = evas_object_rectangle_add(evas_object_evas_get(cd->main_win));
	evas_object_size_hint_weight_set(cd->event_rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_color_set(cd->event_rect, 0, 0, 0, 0);
	elm_win_resize_object_add(cd->main_win, cd->event_rect);
	evas_object_repeat_events_set(cd->event_rect, EINA_TRUE);
	evas_object_show(cd->event_rect);
	evas_object_raise(cd->event_rect);

	cd->gesture_layer = elm_gesture_layer_add(cd->main_win);
	elm_gesture_layer_hold_events_set(cd->gesture_layer, EINA_FALSE);
	elm_gesture_layer_attach(cd->gesture_layer, cd->event_rect);
	elm_gesture_layer_cb_set(cd->gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, flick_end, ad);

	cd->noc_layout = elm_layout_add(cd->main_win);
	elm_layout_theme_set(cd->noc_layout, "layout", "nocontents", "default");
	elm_object_part_text_set(cd->noc_layout, "elm.text", S_NO_ITEMS);

	return cd;
}

void depose_clipdrawer(ClipdrawerData *cd)
{
	utilx_ungrab_key(ecore_x_display_get(), cd->x_main_win, "XF86Back");
	utilx_ungrab_key(ecore_x_display_get(), cd->x_main_win, "XF86Home");
	evas_object_del(cd->main_win);
	evas_object_del(cd->cbhm_popup);
	evas_object_del(cd->popup_conform);
	evas_object_del(cd->popup_win);
	if (cd->anim_timer)
		ecore_timer_del(cd->anim_timer);
	if (cd->keydown_handler)
		ecore_event_handler_del(cd->keydown_handler);
	if (cd->event_rect)
		evas_object_del(cd->event_rect);
	if (cd->gesture_layer)
	{
		elm_gesture_layer_cb_set(cd->gesture_layer, ELM_GESTURE_N_FLICKS, ELM_GESTURE_STATE_END, NULL, NULL);
		evas_object_del(cd->gesture_layer);
	}
	free(cd);
}

static Eina_Bool clipdrawer_add_item(AppData *ad, CNP_ITEM *item)
{
	Eina_Bool duplicated = EINA_FALSE;
	ClipdrawerData *cd = ad->clipdrawer;
	CNP_ITEM *gitem_data = NULL;
	Eina_List *l = NULL;
	Eina_List *l_next = NULL;
	Eina_List *itemlist = eina_list_next(ad->item_list);

	if (!item) return EINA_FALSE;

	EINA_LIST_FOREACH_SAFE(itemlist, l, l_next, gitem_data)
	{
		if ((gitem_data->type_index == item->type_index) && (!SAFE_STRCMP(item->data, gitem_data->data)))
		{
			DBG("duplicated data = %s", (char *)item->data);
			duplicated = EINA_TRUE;
			item_delete_by_CNP_ITEM(ad, gitem_data);
			break;
		}
	}

	if(item->locked)
		cd->locked_item_count++;

	if (item->gitem_style == GRID_ITEM_STYLE_TEXT)
		item->gitem = elm_gengrid_item_prepend(cd->gengrid, &cd->gic_text, item, _gengrid_select_cb, item);
	else if (item->gitem_style == GRID_ITEM_STYLE_IMAGE)
		item->gitem = elm_gengrid_item_prepend(cd->gengrid, &cd->gic_image, item, _gengrid_select_cb, item);
	else
		item->gitem = elm_gengrid_item_prepend(cd->gengrid, &cd->gic_combined, item, _gengrid_select_cb, item);

	return duplicated;
}

static Eina_Bool clipdrawer_del_item(AppData *ad, CNP_ITEM *item)
{
	if (item->gitem)
	{
		elm_object_item_del(item->gitem);
		item->gitem = NULL;
	}
	return EINA_TRUE;
}

static char *_grid_text_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;
	char text_to_show[BUFF];
	if (!item)
		return NULL;

	if (!SAFE_STRCMP(part, "elm.text")) /* text */
	{
		char *entry_text = string_for_entry_get(item->ad, item->type_index, item->data);

		/* limiting the grid text to show to avoid UI blockage in case of large text */
		if (entry_text)
		{
			SAFE_STRNCPY(text_to_show, entry_text, (BUFF - 1));
			strcat(text_to_show, "\0");
		}

		if (entry_text)
			return strdup(text_to_show);
		else
			return SAFE_STRDUP(item->data);
	}
	return NULL;
}

static Evas_Object *_grid_content_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;

	if (!SAFE_STRCMP(part, "delbtn/img")) /* text */
	{
		Evas_Object *btn = elm_button_add(obj);
		elm_object_style_set(btn, "delete_icon");
		evas_object_propagate_events_set(btn,EINA_FALSE);
		evas_object_smart_callback_add(btn, "clicked", _grid_item_button_clicked, item);
		return btn;
	}
	else
		return NULL;
}


static Evas_Object *_grid_image_content_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;
	AppData *ad;
	ClipdrawerData *cd;
	Elm_Object_Item *gitem;
	Evas_Object *sicon;

	if (!item)
		return NULL;

	ad = item->ad;
	cd = ad->clipdrawer;
	gitem = item->gitem;

	if (!SAFE_STRCMP(part, "elm.swallow.content")) /* uri */
	{
		int w, h, iw, ih;
		int grid_image_real_w = cd->grid_image_item_w;
		int grid_image_real_h = cd->grid_image_item_h;
		//double scale;

		sicon = evas_object_image_filled_add(evas_object_evas_get(obj));
		evas_object_image_load_size_set(sicon, grid_image_real_w, grid_image_real_h);
		evas_object_image_file_set(sicon, item->file, NULL);
		evas_object_image_preload(sicon, EINA_FALSE);
		evas_object_image_size_get(sicon, &w, &h);

		//scale = elm_config_scale_get();

		if (w <= 0 || h <= 0)
			return NULL;

		if (w > grid_image_real_w || h > grid_image_real_h)
		{
			if (w > h)
			{
				iw = cd->grid_image_item_w;
				ih = cd->grid_image_item_h;
			}
			else if (w < h)
			{
				iw = (float)cd->grid_image_item_h / 1.5;
				ih = cd->grid_image_item_h;
			}
			else
			{
				iw = cd->grid_image_item_h;
				ih = cd->grid_image_item_h;
			}
		}
		else
		{
			iw = w;
			ih = h;
		}

		evas_object_resize(sicon, iw, ih);
		evas_object_size_hint_min_set(sicon, iw, ih);

		if (cd->paste_text_only)
			elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
		else
			elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
	}
	else if (!SAFE_STRCMP(part, "delbtn/img")) /* text */
	{
		Evas_Object *btn = elm_button_add(obj);
		elm_object_style_set(btn, "delete_icon");
		evas_object_propagate_events_set(btn,EINA_FALSE);
		evas_object_smart_callback_add(btn, "clicked", _grid_item_button_clicked, item);
		return btn;
	}
	else
		return NULL;

	return sicon;
}

static Evas_Object *_grid_combined_content_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;
	AppData *ad;
	ClipdrawerData *cd;
	Elm_Object_Item *gitem;
	Evas_Object *sicon;
	char *entry_text = NULL;

	if (!item)
		return NULL;

	ad = item->ad;
	cd = ad->clipdrawer;
	gitem = item->gitem;

	if (!SAFE_STRCMP(part, "elm.swallow.content")) /* uri */
	{
		int w, h, iw, ih;
		int grid_image_real_w = cd->grid_image_item_w;
		int grid_image_real_h = cd->grid_image_item_h;

		sicon = evas_object_image_filled_add(evas_object_evas_get(obj));
		evas_object_image_load_size_set(sicon, grid_image_real_w, grid_image_real_h);
		evas_object_image_file_set(sicon, item->file, NULL);
		evas_object_image_preload(sicon, EINA_FALSE);
		evas_object_image_size_get(sicon, &w, &h);

		if (w <= 0 || h <= 0)
			return NULL;

		if (w > cd->grid_combined_item_w || h > cd->grid_combined_item_h )
		{
			if (h >= w)
			{
				iw = (float)grid_image_real_h / h * w;
				if (iw > cd->grid_combined_item_w)
				{
					ih = (float)cd->grid_combined_item_w / w * h;
					iw = cd->grid_combined_item_w;
				}
				else
				{
					ih = cd->grid_combined_item_h;
				}
			}
			else
			{
				ih = (float)cd->grid_combined_item_w / w * h;
				iw = cd->grid_combined_item_w;
			}
		}
		else
		{
			iw = w;
			ih = h;
		}

		evas_object_resize(sicon, iw, ih);
		evas_object_size_hint_min_set(sicon, iw, ih);
		entry_text = string_for_entry_get(item->ad, item->type_index, item->data);

		if (entry_text)
		{
			entry_text = evas_textblock_text_markup_to_utf8(NULL, entry_text);
			if (entry_text)
			  strcat(entry_text, "\0");
		}

		if (cd->paste_text_only && entry_text && !strlen(entry_text))
			elm_object_item_signal_emit(gitem, "elm,state,show,dim", "elm");
		else
			elm_object_item_signal_emit(gitem, "elm,state,hide,dim", "elm");
		if (entry_text)
		  free(entry_text);
	}
	else if (!SAFE_STRCMP(part, "delbtn/img")) /* text */
	{
		Evas_Object *btn = elm_button_add(obj);
		elm_object_style_set(btn, "delete_icon");
		evas_object_propagate_events_set(btn,EINA_FALSE);
		evas_object_smart_callback_add(btn, "clicked", _grid_item_button_clicked, item);
		return btn;
	}
	else
		return NULL;

	return sicon;
}

static void _grid_realized(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *gitem = (Elm_Object_Item *) event_info;
	CNP_ITEM *item;

	if (gitem)
	{
		item = elm_object_item_data_get(gitem);

	   if (delete_mode)
	      elm_object_item_signal_emit(gitem, "elm,state,show,delbtn", "elm");
	   else
	      elm_object_item_signal_emit(gitem, "elm,state,hide,delbtn", "elm");
	}
}

void _create_cbhm_popup(AppData *ad, ClipdrawerData *cd)
{
	Evas_Object *btn1;
	Evas_Object *btn2;

	cd->cbhm_popup = elm_popup_add(cd->popup_win);
	elm_object_part_text_set(cd->cbhm_popup, "title,text", S_DELETE_Q);
	elm_object_text_set(cd->cbhm_popup, S_DELETE_ALL_Q);

	btn1 = elm_button_add(cd->cbhm_popup);
	elm_object_style_set(btn1, "popup");
	elm_object_text_set(btn1, S_CANCEL);
	elm_object_part_content_set(cd->cbhm_popup, "button1", btn1);
	evas_object_smart_callback_add(btn1, "clicked", _cancel_btn_cb, ad);

	btn2 = elm_button_add(cd->cbhm_popup);
	elm_object_style_set(btn2, "popup");
	elm_object_text_set(btn2, S_DELETE);
	elm_object_part_content_set(cd->cbhm_popup, "button2", btn2);
	evas_object_smart_callback_add(btn2, "clicked", _ok_btn_cb, ad);
}

static void _show_cbhm_popup(ClipdrawerData *cd)
{
	if (cd == NULL) return;

	if (cd->popup_activate == EINA_TRUE)
		return;

	cd->popup_activate = EINA_TRUE;

	evas_object_show(cd->popup_conform);
	evas_object_show(cd->cbhm_popup);
	evas_object_show(cd->popup_win);
}

static void _hide_cbhm_popup(ClipdrawerData *cd)
{
	if (cd == NULL) return;

	if (cd->popup_activate == EINA_FALSE)
		return;

	cd->popup_activate = EINA_FALSE;

	evas_object_hide(cd->popup_win);
	evas_object_hide(cd->popup_conform);
	evas_object_hide(cd->cbhm_popup);
}

static void clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	AppData *ad = data;

	if (ad->clipdrawer->anim_status != STATUS_NONE)
		return;

	if (!SAFE_STRNCMP(source, EDJE_CLOSE_PART_PREFIX, SAFE_STRLEN(EDJE_CLOSE_PART_PREFIX)))
		clipdrawer_lower_view(ad);
	else if (!SAFE_STRNCMP(source, EDJE_DELETE_MODE_PREFIX, SAFE_STRLEN(EDJE_DELETE_MODE_PREFIX)))
		_delete_mode_set(ad, !delete_mode);
	else if (!SAFE_STRNCMP(source, EDJE_DELETE_ALL_BTN_PART_PREFIX, SAFE_STRLEN(EDJE_DELETE_ALL_BTN_PART_PREFIX)))
		 _show_cbhm_popup(ad->clipdrawer);
	else
		return;
}
static void set_focus_for_app_window(Ecore_X_Window x_main_win, Eina_Bool enable)
{
	    CALLED();
	    Eina_Bool accepts_focus;
	    Ecore_X_Window_State_Hint initial_state;
	    Ecore_X_Pixmap icon_pixmap;
	    Ecore_X_Pixmap icon_mask;
	    Ecore_X_Window icon_window;
	    Ecore_X_Window window_group;
	    Eina_Bool is_urgent;

	    ecore_x_icccm_hints_get (x_main_win,
        &accepts_focus, &initial_state, &icon_pixmap, &icon_mask, &icon_window, &window_group, &is_urgent);
	    ecore_x_icccm_hints_set (x_main_win,
        enable, initial_state, icon_pixmap, icon_mask, icon_window, window_group, is_urgent);
	    DBG("set focus mode = %d", enable);
}


static void _ok_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;
	Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);

	while (gitem)
	{
		CNP_ITEM *gitem_data = elm_object_item_data_get(gitem);
		gitem = elm_gengrid_item_next_get(gitem);

		if(!gitem_data->locked)
			item_delete_by_CNP_ITEM(ad, gitem_data);
	}

	if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
		clipdrawer_lower_view(ad);
	else
		_hide_cbhm_popup(ad->clipdrawer);
}

static void _cancel_btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;

	_hide_cbhm_popup(ad->clipdrawer);
}

static void _grid_item_button_clicked(void *data, Evas_Object *obj, void *event_info)
{
	CNP_ITEM *item = data;
	AppData *ad = item->ad;
	ClipdrawerData *cd = ad->clipdrawer;

	if (cd->anim_status != STATUS_NONE)
		return;

	item_delete_by_CNP_ITEM(ad, item);
}

static Ecore_X_Window isf_ise_window_get()
{
	Ecore_X_Atom   x_atom_isf_control = ecore_x_atom_get("_ISF_CONTROL_WINDOW");
	Ecore_X_Atom   x_atom_isf_ise     = ecore_x_atom_get("_ISF_ISE_WINDOW");
	Ecore_X_Window x_isf_control_win = 0;
	Ecore_X_Window x_isf_ise_win     = 0;
	unsigned char *buf = NULL;
	int            num = 0;
	int            ret;

	ret = ecore_x_window_prop_property_get(0, x_atom_isf_control, ECORE_X_ATOM_WINDOW, 0, &buf, &num);
	if (ret && num)
		memcpy(&x_isf_control_win, buf, sizeof(Ecore_X_Window));
	if (buf)
		free(buf);
	if (!x_isf_control_win)
		return 0;

	ret = ecore_x_window_prop_property_get(x_isf_control_win, x_atom_isf_ise, ECORE_X_ATOM_WINDOW, 0, &buf, &num);
	if (ret && num)
		memcpy(&x_isf_ise_win, buf, sizeof(Ecore_X_Window));
	if (buf)
		free(buf);

	return x_isf_ise_win;
}

void set_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win)
{
	ecore_x_icccm_transient_for_set(x_main_win, x_active_win);
	ecore_x_event_mask_set(x_active_win,
								  ECORE_X_EVENT_MASK_WINDOW_PROPERTY |
								  ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
}

void unset_transient_for(Ecore_X_Window x_main_win)
{
	Ecore_X_Window x_transient_win = ecore_x_icccm_transient_for_get(x_main_win);

	if (x_transient_win) {
		ecore_x_event_mask_unset(x_transient_win,
										 ECORE_X_EVENT_MASK_WINDOW_PROPERTY |
										 ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
		ecore_x_icccm_transient_for_unset(x_main_win);
	}
}

void setting_win(Ecore_X_Display *x_disp, Ecore_X_Window x_root_win, Ecore_X_Window x_main_win)
{
	CALLED();

	Ecore_X_Atom ATOM_WINDOW_EFFECT_ENABLE = 0;
	unsigned int effect_state = 0; // 0 : disabled effect // 1: enable effect

	// disable window effect
	ATOM_WINDOW_EFFECT_ENABLE = ecore_x_atom_get("_NET_CM_WINDOW_EFFECT_ENABLE");
	if (ATOM_WINDOW_EFFECT_ENABLE)
	{
		ecore_x_window_prop_card32_set(x_main_win, ATOM_WINDOW_EFFECT_ENABLE, &effect_state, 1);
	}
	else
	{
		// error case
		ERR("Could not get _NET_CM_WINDOW_EFFECT_ENABLE ATOM");
	}

	ecore_x_icccm_name_class_set(x_main_win, "NORMAL_WINDOW", "NORMAL_WINDOW");

	set_focus_for_app_window(x_main_win, EINA_FALSE);
	ecore_x_window_prop_property_set(
			x_root_win, ecore_x_atom_get("CBHM_ELM_WIN"),
			ECORE_X_ATOM_WINDOW, 32, &x_main_win, 1);
	ecore_x_flush();
}

Evas_Object *create_win(ClipdrawerData *cd, const char *name)
{
	CALLED();

	Evas_Object *win = elm_win_add(NULL, name, ELM_WIN_UTILITY);
	if (!win)
	{
		ERR("elm_win_add return NULL");
		return NULL;
	}
	elm_win_title_set(win, name);
	elm_win_borderless_set(win, EINA_TRUE);
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &cd->root_w, &cd->root_h);
	DBG("root_w: %d, root_h: %d", cd->root_w, cd->root_h);
	//evas_object_resize(win, cd->root_w, cd->root_h);

	//elm_config_scale_set((double)cd->root_w/DEFAULT_WIDTH);
	return win;
}

static void set_sliding_win_geometry(AppData *ad)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	Evas_Coord x, y, w, h;
	int angle = elm_win_rotation_get(cd->main_win);

	if (!ad->x_active_win)
	{
		ERR("x_active_win is NULL");
		return;
	}

	if (angle == 90 || angle == 270)
	{
		h = cd->landscape_height;
		x = 0;
		y = cd->root_w - h;
		w = cd->root_h;
	}
	else
	{
		h = cd->height;
		x = 0;
		y = cd->root_h - h;
		w = cd->root_w;
	}

	if (!h)
		w = 0;

	DBG("[CBHM] change degree geometry... (%d, %d, %d x %d)", x, y, w, h);

	ecore_x_e_illume_clipboard_geometry_set(ad->x_active_win, x, y, w, h);
}

void set_rotation_to_clipdrawer(AppData *ad)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	int angle = elm_win_rotation_get(cd->main_win);
	int x, y, w, h;

	if (angle == 180) // reverse
	{
		h = cd->height;
		x = 0;
		y = 0;
		w = cd->root_w;
	}
	else if (angle == 90) // right rotate
	{
		h = cd->landscape_height;
		x = cd->root_w - h;
		y = 0;
		w = cd->root_h;
	}
	else if (angle == 270) // left rotate
	{
		h = cd->landscape_height;
		x = 0;
		y = 0;
		w = cd->root_h;
	}
	else // angle == 0
	{
		h = cd->height;
		x = 0;
		y = cd->root_h - h;
		w = cd->root_w;
 	}

	evas_object_resize(cd->main_win, w, h);
	evas_object_move(cd->main_win, x, y);
	set_sliding_win_geometry(ad);
}
/*
static Eina_Bool _get_anim_pos(ClipdrawerData *cd, int *sp, int *ep)
{
	if (!sp || !ep)
		return EINA_FALSE;

	int angle = cd->o_degree;
	int anim_start, anim_end;

	if (angle == 180) // reverse
	{
		anim_start = -(cd->root_h - cd->height);
		anim_end = 0;
	}
	else if (angle == 90) // right rotate
	{
		anim_start = cd->root_w;
		anim_end = anim_start - cd->landscape_height;
	}
	else if (angle == 270) // left rotate
	{
		anim_start = -(cd->root_w - cd->landscape_height);
		anim_end = 0;
	}
	else // angle == 0
	{
		anim_start = cd->root_h;
		anim_end = anim_start - cd->height;
	}

	*sp = anim_start;
	*ep = anim_end;
	return EINA_TRUE;
}

static Eina_Bool _do_anim_delta_pos(ClipdrawerData *cd, int sp, int ep, int ac, int *dp)
{
	if (!dp)
		return EINA_FALSE;

	int angle = cd->o_degree;
	int delta;
	double posprop;
	posprop = 1.0*ac/ANIM_DURATION;

	if (angle == 180) // reverse
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(cd->main_win, 0, sp+delta);
	}
	else if (angle == 90) // right rotate
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(cd->main_win, sp+delta, 0);
	}
	else if (angle == 270) // left rotate
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(cd->main_win, sp+delta, 0);
	}
	else // angle == 0
	{
		delta = (int)((sp-ep)*posprop);
		evas_object_move(cd->main_win, 0, sp-delta);
	}
	
	*dp = delta;

	return EINA_TRUE;
}

static void stop_animation(AppData *ad)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	cd->anim_status = STATUS_NONE;
	if (cd->anim_timer)
	{
		ecore_timer_del(cd->anim_timer);
		cd->anim_timer = NULL;
	}

	set_sliding_win_geometry(ad);
}

static Eina_Bool anim_pos_calc_cb(void *data)
{
	AppData *ad = data;
	ClipdrawerData *cd = ad->clipdrawer;
	int anim_start, anim_end, delta;

	_get_anim_pos(cd, &anim_start, &anim_end);

	if (cd->anim_status == SHOW_ANIM)
	{
		if (cd->anim_count > ANIM_DURATION)
		{
			cd->anim_count = ANIM_DURATION;
			stop_animation(ad);
			return EINA_FALSE;
		}
		_do_anim_delta_pos(cd, anim_start, anim_end, cd->anim_count, &delta);
		if (cd->anim_count == 1)
			evas_object_show(cd->main_win);
		cd->anim_count++;
	}
	else if (cd->anim_status == HIDE_ANIM)
	{
		if (cd->anim_count < 0)
		{
			cd->anim_count = 0;
			elm_object_signal_emit(cd->main_layout, "elm,state,hide,historyitems", "elm");
			edje_object_message_signal_process(elm_layout_edje_get(cd->main_layout));
			evas_object_hide(cd->main_win);
			elm_win_lower(cd->main_win);
			unset_transient_for(cd->x_main_win);
			stop_animation(ad);
			_delete_mode_set(ad, EINA_FALSE);
			return EINA_FALSE;
		}
		_do_anim_delta_pos(cd, anim_start, anim_end, cd->anim_count, &delta);
		cd->anim_count--;
	}
	else
	{
		stop_animation(ad);
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static Eina_Bool clipdrawer_anim_effect(AppData *ad, AnimStatus atype)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	if (atype == cd->anim_status)
	{
		WRN("Warning: Animation effect is already in progress.");
		return EINA_FALSE;
	}

	cd->anim_status = atype;

	if (cd->anim_timer)
		ecore_timer_del(cd->anim_timer);
	cd->anim_timer = ecore_timer_add(ANIM_FLOPS, anim_pos_calc_cb, ad);

	return EINA_TRUE;
}
*/
static Eina_Bool timer_cb(void *data)
{
	ClipdrawerData *cd = data;
	/* If the gengrid has some images, evas would have some heavy works for image drawings.
		In this case, window would be delayed in showing up.
		In order to avoid this problem,
		we show the gengrid after window is shown.
	 */
	elm_object_signal_emit(cd->main_layout, "elm,state,show,historyitems", "elm");
	return ECORE_CALLBACK_CANCEL;
}

static void rotate_cb(void *data, Evas_Object * obj, void *event)
{
	if (!data) return;

	AppData *ad= data;

	set_rotation_to_clipdrawer(ad);
}

void clipdrawer_activate_view(AppData* ad)
{
	CALLED();
	ClipdrawerData                *cd = ad->clipdrawer;
	Ecore_X_Window                 x_transient_win = ad->x_active_win;
	Ecore_X_Window                 x_isf_ise_win = 0;
	Ecore_X_Virtual_Keyboard_State isf_ise_state;

	if(cd->main_layout)
	{
		if ((item_count_get(ad, ATOM_INDEX_COUNT_ALL) - cd->locked_item_count) == 0)
			elm_object_signal_emit(cd->main_layout, "elm,state,disable,del", "elm");

		elm_object_part_text_set(cd->main_layout, "panel_title", S_CLIPBOARD);
		elm_object_part_text_set(cd->main_layout, "panel_function_delete_all", S_DELETE_ALL);
		elm_object_part_text_set(cd->main_layout, "panel_function_delete", S_DELETE);
	}

	if (cd->main_win)
	{
		isf_ise_state = ecore_x_e_virtual_keyboard_state_get(ad->x_active_win);
		if (isf_ise_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON) {
			x_isf_ise_win = isf_ise_window_get();
			if (x_isf_ise_win) {
				x_transient_win = x_isf_ise_win;
				ecore_x_e_illume_clipboard_state_set(x_transient_win, ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
			}
		}
		set_transient_for(cd->x_main_win, x_transient_win);

		_delete_mode_set(ad, EINA_FALSE);
		set_rotation_to_clipdrawer(ad);
		evas_object_show(cd->main_win);
		elm_win_activate(cd->main_win);
		ecore_x_e_illume_clipboard_state_set(ad->x_active_win, ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
		utilx_grab_key(ad->x_disp, cd->x_main_win, "XF86Back", TOP_POSITION_GRAB);
		utilx_grab_key(ad->x_disp, cd->x_main_win, "XF86Home", SHARED_GRAB);
		ecore_timer_add(0.125, timer_cb, cd);  //0.125 is experimentally decided.
	}

	if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
	{
		elm_object_part_content_unset(cd->main_layout, "historyitems");
		evas_object_hide(cd->gengrid);
		elm_object_part_content_set(cd->main_layout, "historyitems", cd->noc_layout);
	}
	else
	{
		elm_object_part_content_unset(cd->main_layout, "historyitems");
		evas_object_hide(cd->noc_layout);
		elm_object_part_content_set(cd->main_layout, "historyitems", cd->gengrid);
	}
}

void clipdrawer_lower_view(AppData* ad)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	Ecore_X_Window                 x_isf_ise_win = 0;
	Ecore_X_Virtual_Keyboard_State isf_ise_state;

	if (cd->main_win)
	{
		if(cd->popup_activate)
			_hide_cbhm_popup(ad->clipdrawer);

		elm_object_signal_emit(cd->main_layout, "elm,state,hide,historyitems", "elm");
		edje_object_message_signal_process(elm_layout_edje_get(cd->main_layout));
		Elm_Object_Item *it = elm_gengrid_first_item_get (cd->gengrid);
		elm_gengrid_item_show(it, ELM_GENGRID_ITEM_SCROLLTO_NONE);
		evas_object_hide(cd->main_win);
		elm_win_lower(cd->main_win);
		unset_transient_for(cd->x_main_win);
		_delete_mode_set(ad, EINA_FALSE);

		ecore_x_e_illume_clipboard_state_set(ad->x_active_win, ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
		ecore_x_e_illume_clipboard_geometry_set(ad->x_active_win, 0, 0, 0, 0);

		isf_ise_state = ecore_x_e_virtual_keyboard_state_get(ad->x_active_win);
		if (isf_ise_state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON) {
			x_isf_ise_win = isf_ise_window_get();

			if (x_isf_ise_win)
				ecore_x_e_illume_clipboard_state_set(x_isf_ise_win, ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
		}

		utilx_ungrab_key(ad->x_disp, cd->x_main_win, "XF86Back");
		utilx_ungrab_key(ad->x_disp, cd->x_main_win, "XF86Home");
	}
}

void _delete_mode_set(AppData* ad, Eina_Bool del_mode)
{
	ClipdrawerData *cd = ad->clipdrawer;
	Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);
	CNP_ITEM *item = NULL;

	if (gitem)
		delete_mode = del_mode;
	else
		delete_mode = EINA_FALSE;

	if(!cd->main_layout)
		return;
	if (delete_mode)
	{
		elm_object_part_text_set(cd->main_layout, "panel_function_delete", S_DONE);
		elm_object_signal_emit(cd->main_layout, "elm,state,hide,delete_all", "elm");
	}
	else
	{
		elm_object_part_text_set(cd->main_layout, "panel_function_delete", S_DELETE);

		if (item_count_get(ad, ATOM_INDEX_COUNT_ALL) == 0)
		{
		   elm_object_signal_emit(cd->main_layout, "elm,state,show,delete_all", "elm");
		   elm_object_signal_emit(cd->main_layout, "elm,state,disable,del", "elm");
		   elm_object_part_content_unset(cd->main_layout, "historyitems");
		   evas_object_hide(cd->gengrid);
		   elm_object_part_content_set(cd->main_layout, "historyitems", cd->noc_layout);
		}
		else
		   elm_object_signal_emit(cd->main_layout, "elm,state,show,delete_all", "elm");
	}

	while (gitem)
	{
		item = elm_object_item_data_get(gitem);
		if(!item)
			return;

		if (!item->locked)
		{
			if (delete_mode)
				elm_object_item_signal_emit(gitem, "elm,state,show,delbtn", "elm");
			else
				elm_object_item_signal_emit(gitem, "elm,state,hide,delbtn", "elm");
		}

		gitem = elm_gengrid_item_next_get(gitem);
	}
}
