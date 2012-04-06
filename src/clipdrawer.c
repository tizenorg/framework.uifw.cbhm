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

#define EDJ_PATH "/usr/share/edje"
#define APP_EDJ_FILE EDJ_PATH"/cbhmdrawer.edj"
#define GRP_MAIN "cbhmdrawer"

#define ANIM_DURATION 30 // 1 seconds
#define ANIM_FLOPS (0.5/30)
#define CLIPDRAWER_HEIGHT 360
#define CLIPDRAWER_HEIGHT_LANDSCAPE 228
#define DEFAULT_WIDTH 720
#define GRID_ITEM_SPACE_W 6
#define GRID_ITEM_SINGLE_W 185
#define GRID_ITEM_SINGLE_H 161
#define GRID_ITEM_W (GRID_ITEM_SINGLE_W+(GRID_ITEM_SPACE_W*2))
#define GRID_ITEM_H (GRID_ITEM_SINGLE_H)
#define GRID_IMAGE_LIMIT_W 91
#define GRID_IMAGE_LIMIT_H 113
#define GRID_IMAGE_INNER 10
#define GRID_IMAGE_REAL_W (GRID_ITEM_SINGLE_W - (2*GRID_IMAGE_INNER))
#define GRID_IMAGE_REAL_H (GRID_ITEM_SINGLE_H - (2*GRID_IMAGE_INNER))

static Evas_Object *create_win(ClipdrawerData *cd, const char *name);
static Evas_Object *_grid_content_get(void *data, Evas_Object *obj, const char *part);
static void _grid_del(void *data, Evas_Object *obj);
static Eina_Bool clipdrawer_add_item(AppData *ad, CNP_ITEM *item);
static Eina_Bool clipdrawer_del_item(AppData *ad, CNP_ITEM *item);
static void clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _grid_item_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source);
static void setting_win(Ecore_X_Display *x_disp, Ecore_X_Window x_main_win);
static void set_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win);
static void unset_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win);

static void _change_gengrid_paste_textonly_mode(ClipdrawerData *cd)
{
	CNP_ITEM *item = NULL;

	Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);

	while (gitem)
	{
		item = elm_object_item_data_get(gitem);
		if ((item->type_index == ATOM_INDEX_IMAGE) && (item->layout))
		{
			if (cd->paste_text_only)
				edje_object_signal_emit(elm_layout_edje_get(item->layout), "elm,state,show,dim", "elm");
			else
				edje_object_signal_emit(elm_layout_edje_get(item->layout), "elm,state,hide,dim", "elm");
		}
		gitem = elm_gengrid_item_next_get(gitem);
	}
}

void clipdrawer_paste_textonly_set(AppData *ad, Eina_Bool textonly)
{
	ClipdrawerData *cd = ad->clipdrawer;
	if (cd->paste_text_only != textonly)
		cd->paste_text_only = textonly;
	DTRACE("paste textonly mode = %d\n", textonly);

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
		DMSG("ERROR: elm_layout_add return NULL\n");
		return NULL;
	}

	if (!elm_layout_file_set(layout, file, group))
	{
		DMSG("ERROR: elm_layout_file_set return FALSE\n");
		evas_object_del(layout);
		return NULL;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, layout);

	evas_object_show(layout);
	return layout;
}

static Eina_Bool keydown_cb(void *data, int type, void *event)
{
	AppData *ad = data;
	Ecore_Event_Key *ev = event;
	if (!strcmp(ev->keyname, KEY_END))
		clipdrawer_lower_view(ad);

	return ECORE_CALLBACK_PASS_ON;
}

ClipdrawerData* init_clipdrawer(AppData *ad)
{
	ClipdrawerData *cd = calloc(1, sizeof(ClipdrawerData));

	/* create and setting window */
	if (!cd)
		return NULL;
	if (!(cd->main_win = create_win(cd, APPNAME)))
	{
		free(cd);
		return NULL;
	}
	cd->x_main_win = elm_win_xwindow_get(cd->main_win);
	setting_win(ad->x_disp, cd->x_main_win);

	/* edj setting */
	if (!(cd->main_layout = _load_edj(cd->main_win, APP_EDJ_FILE, GRP_MAIN)))
	{
		evas_object_del(cd->main_win);
		free(cd);
		return NULL;
	}

	/* create and setting gengrid */
	elm_theme_extension_add(NULL, APP_EDJ_FILE);
	edje_object_signal_callback_add(elm_layout_edje_get(cd->main_layout),
			"mouse,up,1", "*", clipdrawer_ly_clicked, ad);

	cd->gengrid = elm_gengrid_add(cd->main_win);
	elm_object_part_content_set(cd->main_layout, "historyitems", cd->gengrid);
	elm_gengrid_item_size_set(cd->gengrid, GRID_ITEM_W+2, GRID_ITEM_H);
	elm_gengrid_align_set(cd->gengrid, 0.5, 0.0);
	elm_gengrid_horizontal_set(cd->gengrid, EINA_TRUE);
	elm_gengrid_bounce_set(cd->gengrid, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(cd->gengrid, EINA_FALSE);
//	evas_object_smart_callback_add(cd->gengrid, "selected", _grid_click_paste, ad);
	evas_object_size_hint_weight_set(cd->gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_gengrid_clear(cd->gengrid);

	cd->gic.item_style = "default_grid";
	cd->gic.func.text_get = NULL;
	cd->gic.func.content_get = _grid_content_get;
	cd->gic.func.state_get = NULL;
	cd->gic.func.del = _grid_del;

	evas_object_show(cd->gengrid);

	ad->draw_item_add = clipdrawer_add_item;
	ad->draw_item_del = clipdrawer_del_item;
//	ad->x_main_win = cd->x_main_win;

	cd->keydown_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, keydown_cb, ad);
	cd->evas = evas_object_evas_get(cd->main_win);

	return cd;
}

void depose_clipdrawer(ClipdrawerData *cd)
{
	evas_object_del(cd->main_win);
	if (cd->anim_timer)
		ecore_timer_del(cd->anim_timer);
	if (cd->keydown_handler)
		ecore_event_handler_del(cd->keydown_handler);
	free(cd);
}

static Eina_Bool clipdrawer_add_item(AppData *ad, CNP_ITEM *item)
{
	ClipdrawerData *cd = ad->clipdrawer;
	if (item->type_index == ATOM_INDEX_IMAGE)
	{
		Elm_Object_Item *gitem = elm_gengrid_first_item_get(cd->gengrid);
		while (gitem)
		{
			CNP_ITEM *gitem_data = elm_object_item_data_get(gitem);
			gitem = elm_gengrid_item_next_get(gitem);
			if ((gitem_data->type_index == item->type_index) && (!strcmp(item->data, gitem_data->data)))
			{
				DMSG("duplicated file path = %s\n", item->data);
				item_delete_by_CNP_ITEM(ad, gitem_data);
			}
		}
		cd->gic.item_style = "clipboard_photo_style";
	}
	else
	{
		cd->gic.item_style = "default_grid";
	}

	item->gitem = elm_gengrid_item_prepend(cd->gengrid, &cd->gic, item, NULL, NULL);

	return EINA_TRUE;
}

static Eina_Bool clipdrawer_del_item(AppData *ad, CNP_ITEM *item)
{
	if (item->gitem)
		elm_object_item_del(item->gitem);
	return EINA_TRUE;
}

static void _grid_del(void *data, Evas_Object *obj)
{
	CNP_ITEM *item = data;
	item->gitem = NULL;
	item->layout = NULL;
}

static Evas_Object *_grid_content_get(void *data, Evas_Object *obj, const char *part)
{
	CNP_ITEM *item = data;
	AppData *ad = item->ad;
	ClipdrawerData *cd = ad->clipdrawer;

	if (strcmp(part, "elm.swallow.icon"))
		return NULL;
	if (item->type_index == ATOM_INDEX_IMAGE) /* text/uri */
	{
		int w, h, iw, ih;

		Evas_Object *layout = elm_layout_add(obj);
		elm_layout_theme_set(layout, "gengrid", "item", "clipboard_style");
		edje_object_signal_callback_add(elm_layout_edje_get(layout),
				"mouse,up,1", "*", _grid_item_ly_clicked, data);


		Evas_Object *sicon;
		sicon = evas_object_image_add(evas_object_evas_get(obj));
		evas_object_image_load_size_set(sicon, GRID_IMAGE_REAL_W, GRID_IMAGE_REAL_H);
		evas_object_image_file_set(sicon, item->data, NULL);
		evas_object_image_size_get(sicon, &w, &h);

		if (w > GRID_IMAGE_REAL_W || h > GRID_IMAGE_REAL_H)
		{
			if (w >= h)
			{
				iw = GRID_IMAGE_REAL_W;
				ih = (float)GRID_IMAGE_REAL_W / w * h;
			}
			else
			{
				iw = (float)GRID_IMAGE_REAL_H / h * w;
				ih = GRID_IMAGE_REAL_H;
			}
		}
		else
		{
			iw = w;
			ih = h;
		}

		evas_object_image_fill_set(sicon, 0, 0, iw, ih);
		evas_object_resize(sicon, iw, ih);
		evas_object_size_hint_min_set(sicon, iw, ih);
		elm_object_part_content_set(layout, "elm.swallow.icon", sicon);

		if (cd->paste_text_only)
			edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,show,dim", "elm");
		else
			edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,hide,dim", "elm");

		item->layout = layout;
	}
	else
	{
		Evas_Object *layout = elm_layout_add(obj);
		elm_layout_theme_set(layout, "gengrid", "widestyle", "horizontal_layout");
		edje_object_signal_callback_add(elm_layout_edje_get(layout), 
				"mouse,up,1", "*", _grid_item_ly_clicked, data);
		Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(obj));
		evas_object_resize(rect, GRID_ITEM_W, GRID_ITEM_H);
		evas_object_color_set(rect, 242, 233, 183, 255);
		evas_object_show(rect);
		elm_object_part_content_set(layout, "elm.swallow.icon", rect);

		Evas_Object *ientry = elm_entry_add(obj);
		evas_object_size_hint_weight_set(ientry, 0, 0);
		elm_entry_scrollable_set(ientry, EINA_TRUE);

		char *entry_text = string_for_entry_get(ad, item->type_index, item->data);
		if (entry_text)
		{
			elm_object_part_text_set(ientry, NULL, entry_text);
			free(entry_text);
		}
		else
		{
			elm_object_part_text_set(ientry, NULL, item->data);
		}
		elm_entry_editable_set(ientry, EINA_FALSE);
		elm_entry_context_menu_disabled_set(ientry, EINA_TRUE);
		evas_object_show(ientry);
		elm_object_part_content_set(layout, "elm.swallow.inner", ientry);

		item->layout = layout;
	}

	return item->layout;
}

static void clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	AppData *ad = data;

	if (ad->clipdrawer->anim_status != STATUS_NONE)
		return;

#define EDJE_CLOSE_PART_PREFIX "background/close"
	if (!strncmp(source, EDJE_CLOSE_PART_PREFIX, strlen(EDJE_CLOSE_PART_PREFIX)))
	{
		clipdrawer_lower_view(ad);
	}
}

static void _grid_del_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	CNP_ITEM *item = data;
	AppData *ad = item->ad;
	ClipdrawerData *cd = ad->clipdrawer;
	const char *label = elm_object_item_text_get(event_info);

	/* delete popup */
	evas_object_del(obj);

	if (!strcmp(label, "Yes"))
	{
		item_delete_by_CNP_ITEM(ad, item);
	}
}

static void _grid_item_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	CNP_ITEM *item = data;
	AppData *ad = item->ad;
	ClipdrawerData *cd = ad->clipdrawer;

	if (cd->anim_status != STATUS_NONE)
		return;

	Elm_Object_Item *sgobj = NULL;
	sgobj = elm_gengrid_selected_item_get(cd->gengrid);
	item = elm_object_item_data_get(sgobj);

	if (!sgobj || !item)
	{
		DTRACE("ERR: cbhm can't get the selected item\n");
		return;
	}

	#define EDJE_DELBTN_PART_PREFIX "delbtn"
	if (strncmp(source, EDJE_DELBTN_PART_PREFIX, strlen(EDJE_DELBTN_PART_PREFIX)))
	{
		elm_gengrid_item_selected_set(sgobj, EINA_FALSE);

		if (item->type_index != ATOM_INDEX_IMAGE || !cd->paste_text_only)
		{
			set_selection_owner(ad, ECORE_X_SELECTION_SECONDARY, item);
		}
	}
	else
	{
		elm_gengrid_item_selected_set(sgobj, EINA_FALSE);

		Evas_Object *popup = elm_popup_add(cd->main_win);
		elm_popup_timeout_set(popup, 5);
		evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_object_text_set(popup, "Are you sure delete this?");
		elm_popup_item_append(popup, "Yes", NULL, _grid_del_response_cb, item);
		elm_popup_item_append(popup, "No", NULL, _grid_del_response_cb, item);
		evas_object_show(popup);
	}
}

void set_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win)
{
	ecore_x_icccm_transient_for_set(x_main_win, x_active_win);
	ecore_x_event_mask_set(x_active_win,
				ECORE_X_EVENT_MASK_WINDOW_PROPERTY | ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
}

void unset_transient_for(Ecore_X_Window x_main_win, Ecore_X_Window x_active_win)
{
	ecore_x_event_mask_unset(x_active_win,
			ECORE_X_EVENT_MASK_WINDOW_PROPERTY | ECORE_X_EVENT_MASK_WINDOW_CONFIGURE);
	ecore_x_icccm_transient_for_unset(x_main_win);
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
	DMSG("set focus mode = %d\n", enable);
}

void setting_win(Ecore_X_Display *x_disp, Ecore_X_Window x_main_win)
{
	CALLED();
	// disable window effect
	utilx_set_window_effect_state(x_disp, x_main_win, 0);

	ecore_x_icccm_name_class_set(x_main_win, "NORMAL_WINDOW", "NORMAL_WINDOW");

	set_focus_for_app_window(x_main_win, EINA_FALSE);

}

Evas_Object *create_win(ClipdrawerData *cd, const char *name)
{
	CALLED();

	Evas_Object *win = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (!win)
	{
		DMSG("ERROR: elm_win_add return NULL\n");
		return NULL;
	}
	elm_win_title_set(win, name);
	elm_win_borderless_set(win, EINA_TRUE);
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &cd->root_w, &cd->root_h);
	DMSG("root_w: %d, root_h: %d\n", cd->root_w, cd->root_h);
	evas_object_resize(win, cd->root_w, cd->root_h);

	elm_scale_set((double)cd->root_w/DEFAULT_WIDTH);
	return win;
}

static void set_sliding_win_geometry(ClipdrawerData *cd)
{
	CALLED();
	Ecore_X_Window zone;
	Evas_Coord x, y, w, h;
	zone = ecore_x_e_illume_zone_get(cd->x_main_win);
	DTRACE(" zone:%x\n", zone);

	if (cd->o_degree == 90 || cd->o_degree == 270)
	{
		h = cd->anim_count * CLIPDRAWER_HEIGHT_LANDSCAPE / ANIM_DURATION;
		x = 0;
		y = cd->root_w - h;
		w = cd->root_h;
	}
	else
	{
		h = cd->anim_count * CLIPDRAWER_HEIGHT / ANIM_DURATION;
		x = 0;
		y = cd->root_h - h;
		w = cd->root_w;
	}

	if (!h)
		w = 0;

	DTRACE("[CBHM] change degree geometry... (%d, %d, %d x %d)\n", x, y, w, h);
	int clipboard_state;
	if (cd->anim_count)
		clipboard_state = ECORE_X_ILLUME_CLIPBOARD_STATE_ON;
	else
		clipboard_state = ECORE_X_ILLUME_CLIPBOARD_STATE_OFF;
	ecore_x_e_illume_clipboard_geometry_set(zone, x, y, w, h);
	ecore_x_e_illume_clipboard_state_set(zone, clipboard_state);
}

void set_rotation_to_clipdrawer(ClipdrawerData *cd)
{
	CALLED();
	int angle = cd->o_degree;
	int x, y, w, h;

	if (angle == 180) // reverse
	{
		h = CLIPDRAWER_HEIGHT;
		x = 0;
		y = 0;
		w = cd->root_w;
	}
	else if (angle == 90) // right rotate
	{
		h = CLIPDRAWER_HEIGHT_LANDSCAPE;
		x = cd->root_w - h;
		y = 0;
		w = cd->root_h;
	}
	else if (angle == 270) // left rotate
	{
		h = CLIPDRAWER_HEIGHT_LANDSCAPE;
		x = 0;
		y = 0;
		w = cd->root_h;
	}
	else // angle == 0
	{
		h = CLIPDRAWER_HEIGHT;
		x = 0;
		y = cd->root_h - h;
		w = cd->root_w;
 	}

	evas_object_resize(cd->main_win, w, h);
	evas_object_move(cd->main_win, x, y);
	if (cd->anim_count == ANIM_DURATION)
		set_sliding_win_geometry(cd);
}

static Eina_Bool _get_anim_pos(ClipdrawerData *cd, int *sp, int *ep)
{
	if (!sp || !ep)
		return EINA_FALSE;

	int angle = cd->o_degree;
	int anim_start, anim_end;

	if (angle == 180) // reverse
	{
		anim_start = -(cd->root_h - CLIPDRAWER_HEIGHT);
		anim_end = 0;
	}
	else if (angle == 90) // right rotate
	{
		anim_start = cd->root_w;
		anim_end = anim_start - CLIPDRAWER_HEIGHT_LANDSCAPE;
	}
	else if (angle == 270) // left rotate
	{
		anim_start = -(cd->root_w - CLIPDRAWER_HEIGHT_LANDSCAPE);
		anim_end = 0;
	}
	else // angle == 0
	{
		anim_start = cd->root_h;
		anim_end = anim_start - CLIPDRAWER_HEIGHT;
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

	set_sliding_win_geometry(cd);
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
			evas_object_hide(cd->main_win);
			elm_win_lower(cd->main_win);
			unset_transient_for(cd->x_main_win, ad->x_active_win);
			stop_animation(ad);
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
		DTRACE("Warning: Animation effect is already in progress. \n");
		return EINA_FALSE;
	}

	cd->anim_status = atype;

	if (cd->anim_timer)
		ecore_timer_del(cd->anim_timer);
	cd->anim_timer = ecore_timer_add(ANIM_FLOPS, anim_pos_calc_cb, ad);

	return EINA_TRUE;
}

void clipdrawer_activate_view(AppData* ad)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	if (cd->main_win)
	{
		set_focus_for_app_window(cd->x_main_win, EINA_TRUE);
		set_transient_for(cd->x_main_win, ad->x_active_win);
		cd->o_degree = get_active_window_degree(ad->x_active_win);
		elm_win_rotation_set(cd->main_win, cd->o_degree);
		set_rotation_to_clipdrawer(cd);
	//	evas_object_show(cd->main_win);
		elm_win_activate(cd->main_win);
	//	if (clipdrawer_anim_effect(ad, SHOW_ANIM))
		clipdrawer_anim_effect(ad, SHOW_ANIM);
	}
}

void clipdrawer_lower_view(AppData* ad)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	if (cd->main_win && cd->anim_count)
	{
		set_focus_for_app_window(cd->x_main_win, EINA_FALSE);
	//	if (clipdrawer_anim_effect(ad, HIDE_ANIM))
	//		ad->windowshow = EINA_FALSE;
		clipdrawer_anim_effect(ad, HIDE_ANIM);
	}
}
