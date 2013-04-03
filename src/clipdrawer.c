/*
 * cbhm
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#define DEFAULT_WIDTH 720

#define MULTI_(id) dgettext("sys_string", #id)
#define S_CLIPBOARD MULTI_(IDS_COM_BODY_CLIPBOARD)
#define S_DELETE MULTI_(IDS_COM_BODY_DELETE)
#define S_DONE MULTI_(IDS_COM_BODY_DONE)

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

	return ECORE_CALLBACK_DONE;
}

ClipdrawerData* init_clipdrawer(AppData *ad)
{
	ClipdrawerData *cd = calloc(1, sizeof(ClipdrawerData));
	const char *data;

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

	double scale = elm_config_scale_get();
	Evas_Object* ly = elm_layout_edje_get(cd->main_layout);

	data = edje_object_data_get(ly, "clipboard_height");
	cd->height = data ? atoi(data) : 0;
	cd->height *= scale;

	data = edje_object_data_get(ly, "clipboard_landscape_height");
	cd->landscape_height = data ? atoi(data) : 0;
	cd->landscape_height *= scale;

	data = edje_object_data_get(ly, "grid_item_bg_w");
	cd->grid_item_bg_w = data ? atoi(data) : 0;
	cd->grid_item_bg_w *= scale;

	data = edje_object_data_get(ly, "grid_item_bg_h");
	cd->grid_item_bg_h = data ? atoi(data) : 0;
	cd->grid_item_bg_h *= scale;

	data = edje_object_data_get(ly, "grid_image_item_w");
	cd->grid_image_item_w = data ? atoi(data) : 0;
	cd->grid_image_item_w *= scale;

	data = edje_object_data_get(ly, "grid_image_item_h");
	cd->grid_image_item_h = data ? atoi(data) : 0;
	cd->grid_image_item_h *= scale;

	/* create and setting gengrid */
	elm_theme_extension_add(NULL, APP_EDJ_FILE);
	edje_object_signal_callback_add(elm_layout_edje_get(cd->main_layout),
			"mouse,up,1", "*", clipdrawer_ly_clicked, ad);

	cd->gengrid = elm_gengrid_add(cd->main_win);
	elm_object_part_content_set(cd->main_layout, "historyitems", cd->gengrid);
	elm_gengrid_item_size_set(cd->gengrid, cd->grid_item_bg_w, cd->grid_item_bg_h);
	elm_gengrid_align_set(cd->gengrid, 0.0, 0.0);
	elm_gengrid_horizontal_set(cd->gengrid, EINA_TRUE);
	elm_gengrid_bounce_set(cd->gengrid, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(cd->gengrid, EINA_FALSE);
//	evas_object_smart_callback_add(cd->gengrid, "selected", _grid_click_paste, ad);
	evas_object_size_hint_weight_set(cd->gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_gengrid_clear(cd->gengrid);

	cd->gic.item_style = "clipboard";
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

	delete_mode = EINA_FALSE;

	return cd;
}

void depose_clipdrawer(ClipdrawerData *cd)
{
	utilx_ungrab_key(ecore_x_display_get(), cd->x_main_win, KEY_END);
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

	if (item->type_index == ATOM_INDEX_IMAGE && !strcmp(part, "elm.swallow.icon")) /* uri */
	{
		int w, h, iw, ih;

		Evas_Object *layout = elm_layout_add(obj);
		elm_layout_file_set(layout, APP_EDJ_FILE, "elm/gengrid/item/clipboard_image/default");
		edje_object_signal_callback_add(elm_layout_edje_get(layout),
				"mouse,up,1", "*", _grid_item_ly_clicked, data);

		int grid_image_real_w = cd->grid_image_item_w;
		int grid_image_real_h = cd->grid_image_item_h;

		Evas_Object *sicon = evas_object_image_add(evas_object_evas_get(obj));
		evas_object_image_load_size_set(sicon, grid_image_real_w, grid_image_real_h);
		evas_object_image_file_set(sicon, item->data, NULL);
		evas_object_image_size_get(sicon, &w, &h);

		if (w <= 0 || h <= 0)
			return NULL;

		if (w > grid_image_real_w || h > grid_image_real_h)
		{
			if (w >= h)
			{
				ih = (float)grid_image_real_w / w * h;
				if (ih > grid_image_real_h)
				{
					iw = (float)grid_image_real_h / h * w;
					ih = grid_image_real_h;
				}
				else
				{
					iw = grid_image_real_w;
				}
			}
			else
			{
				iw = (float)grid_image_real_h / h * w;
				ih = grid_image_real_h;
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
	else if (item->type_index != ATOM_INDEX_IMAGE && !strcmp(part, "elm.swallow.entry")) /* text */
	{
		Evas_Object *layout = elm_layout_add(obj);
		elm_layout_file_set(layout, APP_EDJ_FILE, "elm/gengrid/item/clipboard_text/default");
		edje_object_signal_callback_add(elm_layout_edje_get(layout),
				"mouse,up,1", "*", _grid_item_ly_clicked, data);

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
		elm_object_part_content_set(layout, "elm.swallow.entry", ientry);

		item->layout = layout;
	}
	else
		return NULL;

	if (delete_mode)
		edje_object_signal_emit(elm_layout_edje_get(item->layout), "elm,state,show,delbtn", "elm");
	else
		edje_object_signal_emit(elm_layout_edje_get(item->layout), "elm,state,hide,delbtn", "elm");

	return item->layout;
}

static void clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	AppData *ad = data;

	if (ad->clipdrawer->anim_status != STATUS_NONE)
		return;

#define EDJE_CLOSE_PART_PREFIX "background/title/close/image"
#define EDJE_DELETE_MODE_PREFIX "background/title/delete/image"
	if (!strncmp(source, EDJE_CLOSE_PART_PREFIX, strlen(EDJE_CLOSE_PART_PREFIX)))
	{
		clipdrawer_lower_view(ad);
	}
	else if (!strncmp(source, EDJE_DELETE_MODE_PREFIX, strlen(EDJE_DELETE_MODE_PREFIX)))
	{
		_delete_mode_set(ad, !delete_mode);
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
	if (!sgobj)
	{
		DTRACE("ERR: the gengrid item is unselected\n");
		return;
	}

	item = elm_object_item_data_get(sgobj);
	if (!item)
	{
		DTRACE("ERR: cbhm can't get the selected item\n");
		return;
	}

	#define EDJE_DELBTN_PART_PREFIX "delbtn/img"
	if (strncmp(source, EDJE_DELBTN_PART_PREFIX, strlen(EDJE_DELBTN_PART_PREFIX)))
	{
		elm_gengrid_item_selected_set(sgobj, EINA_FALSE);

		if (delete_mode)
			return;

		if (item->type_index != ATOM_INDEX_IMAGE || !cd->paste_text_only)
		{
			set_selection_owner(ad, ECORE_X_SELECTION_SECONDARY, item);
		}
	}
	else
	{
		elm_gengrid_item_selected_set(sgobj, EINA_FALSE);

		item_delete_by_CNP_ITEM(ad, item);
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
	utilx_grab_key(x_disp, x_main_win, KEY_END, TOP_POSITION_GRAB);
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

	elm_config_scale_set((double)cd->root_w/DEFAULT_WIDTH);
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

	DTRACE("[CBHM] change degree geometry... (%d, %d, %d x %d)\n", x, y, w, h);

	ecore_x_e_illume_clipboard_geometry_set(zone, x, y, w, h);
}

void set_rotation_to_clipdrawer(ClipdrawerData *cd)
{
	CALLED();
	int angle = cd->o_degree;
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

	elm_object_part_text_set(cd->main_layout, "panel_title", S_CLIPBOARD);
	elm_object_part_text_set(cd->main_layout, "panel_function", S_DELETE);

	if (cd->main_win)
	{
		set_transient_for(cd->x_main_win, ad->x_active_win);
		cd->o_degree = get_active_window_degree(ad->x_active_win);
		elm_win_rotation_set(cd->main_win, cd->o_degree);
		set_rotation_to_clipdrawer(cd);
		evas_object_show(cd->main_win);
		elm_win_activate(cd->main_win);
		Ecore_X_Window zone = ecore_x_e_illume_zone_get(cd->x_main_win);
		ecore_x_e_illume_clipboard_state_set(zone, ECORE_X_ILLUME_CLIPBOARD_STATE_ON);
	}
}

void clipdrawer_lower_view(AppData* ad)
{
	CALLED();
	ClipdrawerData *cd = ad->clipdrawer;
	if (cd->main_win)
	{
		evas_object_hide(cd->main_win);
		elm_win_lower(cd->main_win);
		unset_transient_for(cd->x_main_win, ad->x_active_win);
		_delete_mode_set(ad, EINA_FALSE);
		Ecore_X_Window zone = ecore_x_e_illume_zone_get(cd->x_main_win);
		ecore_x_e_illume_clipboard_state_set(zone, ECORE_X_ILLUME_CLIPBOARD_STATE_OFF);
		ecore_x_e_illume_clipboard_geometry_set(zone, 0, 0, 0, 0);
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

	if (delete_mode)
	{
		elm_object_part_text_set(cd->main_layout, "panel_function", S_DONE);
	}
	else
	{
		elm_object_part_text_set(cd->main_layout, "panel_function", S_DELETE);
	}

	while (gitem)
	{
		item = elm_object_item_data_get(gitem);
		if (delete_mode)
			edje_object_signal_emit(elm_layout_edje_get(item->layout), "elm,state,show,delbtn", "elm");
		else
			edje_object_signal_emit(elm_layout_edje_get(item->layout), "elm,state,hide,delbtn", "elm");

		gitem = elm_gengrid_item_next_get(gitem);
	}
}
