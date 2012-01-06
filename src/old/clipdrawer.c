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
#include "storage.h"
#include "xcnphandler.h"
#include "clipdrawer.h"

#define DELETE_ICON_PATH "/usr/share/cbhm/icons/05_delete.png"
#define IM	"/usr/share/cbhm/icons/"
static const char *g_images_path[] = {
	IM"cbhm_default_img.png",
};
#define N_IMAGES (1)

#define GRID_ITEM_SPACE_W 6
#define GRID_ITEM_SINGLE_W 185
#define GRID_ITEM_SINGLE_H 161
#define GRID_ITEM_W (GRID_ITEM_SINGLE_W+(GRID_ITEM_SPACE_W*2))
#define GRID_ITEM_H (GRID_ITEM_SINGLE_H)
#define GRID_IMAGE_LIMIT_W 91
#define GRID_IMAGE_LIMIT_H 113

#define ANIM_DURATION 30 // 1 seconds
#define ANIM_FLOPS (0.5/30)

// gic should live at gengrid callback functions
Elm_Gengrid_Item_Class gic;
static Ecore_Timer *anim_timer = NULL;

typedef struct tag_griditem
{
	int itype;
	Elm_Gengrid_Item *item;
	const char *ipathdata;
	Eina_Strbuf *istrdata;
	Evas_Object *delbtn;
	Evas_Object *ilayout;
} griditem_t;

const char *
remove_tags(const char *p)
{
   char *q,*ret;
   int i;
   if (!p) return NULL;

   q = malloc(strlen(p) + 1);
   if (!q) return NULL;
   ret = q;

   while (*p)
     {
        if ((*p != '<')) *q++ = *p++;
        else if (*p == '<')
          {
             if ((p[1] == 'b') && (p[2] == 'r') &&
                 ((p[3] == ' ') || (p[3] == '/') || (p[3] == '>')))
               *q++ = '\n';
             while ((*p) && (*p != '>')) p++;
             p++;
          }
     }
   *q = 0;

   return ret;
}

const char* clipdrawer_get_plain_string_from_escaped(char *escstr)
{
	/* NOTE : return string should be freed */
	return remove_tags(escstr);
}

static char* _get_string_for_entry(char *str)
{
	if (!str)
		return NULL;

	Eina_Strbuf *strbuf = eina_strbuf_new();
	if (!strbuf)
		return strdup(str);
	eina_strbuf_prepend(strbuf, "<font_size=18><color=#000000FF>");

	char *trail = str;

	while (trail && *trail)
	{
		char *pretrail = trail;
		unsigned long length;
		char *temp;
		char *endtag;

		trail = strchr(trail, '<');
		if (!trail)
		{
			eina_strbuf_append(strbuf, pretrail);
			break;
		}
		endtag = strchr(trail, '>');
		if (!endtag)
			break;

		length = trail - pretrail;

		temp = strndup(pretrail, length);
		if (!temp)
		{
			trail++;
			continue;
		}

		DTRACE("temp str: %s \n", temp);
		eina_strbuf_append(strbuf, temp);
		free(temp);
		trail++;

		if (trail[0] == '/')
		{
			trail = endtag + 1;
			continue;
		}

		if (strncmp(trail, "br", 2) == 0)
		{
			eina_strbuf_append(strbuf, "<br>");
			trail = endtag + 1;
			continue;
		}

		if (strncmp(trail, "img", 3) == 0)
		{
			char *src = strstr(trail, "file://");
			char *src_endtag = strchr(trail, '>');
			if (!src || !src_endtag || src_endtag < src)
				continue;

			length = src_endtag - src;

			src = strndup(src, length);
			if (!src)
			{
				trail = endtag + 1;
				continue;
			}
			temp = src;
			while(*temp)
			{
				if (*temp == '\"' || *temp == '>')
					*temp = '\0';
				else
					temp++;
			}

			eina_strbuf_append_printf(strbuf, "<item absize=66x62 href=%s></item>", src);
			DTRACE("src str: %s \n", src);
			free(src);
		}
		trail = endtag + 1;
	}

	char *ret = eina_strbuf_string_steal(strbuf);
	eina_strbuf_free(strbuf);
	DTRACE("result str: %s \n", ret);
	return ret;
}

static void _grid_del_response_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Gengrid_Item *it = (Elm_Gengrid_Item *)data;
	evas_object_del(obj);

	if((int)event_info == ELM_POPUP_RESPONSE_OK)
	{
		struct appdata *ad = g_get_main_appdata();
		elm_gengrid_item_del(it);
		ad->hicount--;
		if (ad->hicount < 0)
		{
			int cnt = 0;
			Elm_Gengrid_Item *trail = elm_gengrid_first_item_get(ad->hig);
			while(trail)
			{
				cnt++;
				elm_gengrid_item_next_get(trail);
			}
			ad->hicount = cnt;
			DTRACE("ERR: cbhm history cnt < 0, gengrid item cnt: %d\n", cnt);
		}
	}
}

static void _grid_click_delete(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
}

static void
_grid_item_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	struct appdata *ad = g_get_main_appdata();

	if (ad->anim_status != STATUS_NONE)
		return;

	Elm_Gengrid_Item *sgobj = NULL;
	sgobj = elm_gengrid_selected_item_get(ad->hig);
	griditem_t *ti = NULL;
	ti = elm_gengrid_item_data_get(sgobj);

	#define EDJE_DELBTN_PART_PREFIX "delbtn"
	if (strncmp(source, EDJE_DELBTN_PART_PREFIX, strlen(EDJE_DELBTN_PART_PREFIX)))
	{
		if (!sgobj || !ti)
		{
			DTRACE("ERR: cbhm can't get the selected image\n");
			return;
		}

		elm_gengrid_item_selected_set(sgobj, EINA_FALSE);

		if (ti->itype == GI_TEXT)
		{
			char *p = strdup(eina_strbuf_string_get(ti->istrdata));

			elm_selection_set(1, ad->hig, /*ELM_SEL_FORMAT_HTML*/0x10, p);
		}
		else //if (ti->itype == GI_IMAGE)
		{
			if (!clipdrawer_paste_textonly_get(ad))
			{
				int len = strlen(ti->ipathdata);
				char *p = malloc(len + 10);
				snprintf(p,len+10, "file:///%s", ti->ipathdata);

				elm_selection_set(/*secondary*/1, ad->hig,/*ELM_SEL_FORMAT_IMAGE*/4,p);
			}
			else
			{
				DTRACE("ERR: cbhm image paste mode is false\n");
			}
		}
		return;
	}

	if (!sgobj)
	{
		DTRACE("ERR: cbhm can't get the selected item\n");
		return;
	}

	elm_gengrid_item_selected_set(sgobj, EINA_FALSE);

	Evas_Object *popup = elm_popup_add(ad->win_main);
	elm_popup_timeout_set(popup, 5);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_popup_desc_set(popup, "Are you sure delete this?");
	elm_popup_buttons_add(popup, 2,
						  "Yes", ELM_POPUP_RESPONSE_OK,
						  "No", ELM_POPUP_RESPONSE_CANCEL,
						  NULL);
	evas_object_smart_callback_add(popup, "response", _grid_del_response_cb, sgobj);
	evas_object_show(popup);
}

Evas_Object* _grid_icon_get(const void *data, Evas_Object *obj, const char *part)
{
	griditem_t *ti = (griditem_t *)data;

	if (!strcmp(part, "elm.swallow.icon"))
	{
		if (ti->itype == GI_TEXT)
		{
			Evas_Object *layout = elm_layout_add (obj);
			elm_layout_theme_set(layout, "gengrid", "widestyle", "horizontal_layout");
			edje_object_signal_callback_add(elm_layout_edje_get(layout),
											"mouse,up,1", "*", _grid_item_ly_clicked, data);
			Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(obj));
			evas_object_resize(rect, GRID_ITEM_W, GRID_ITEM_H);
			evas_object_color_set(rect, 242, 233, 183, 255);
			evas_object_show(rect);
			elm_layout_content_set(layout, "elm.swallow.icon", rect);

			// FIXME: add string length check
			Evas_Object *ientry = elm_entry_add(obj);
			evas_object_size_hint_weight_set(ientry, 0, 0);
			Eina_Strbuf *strent = NULL;
			char *strdata = eina_strbuf_string_get(ti->istrdata);
			int i, skipflag, strcnt;

			strent = eina_strbuf_new();
			skipflag = 0;
			strcnt = 0;
			for (i = 0; i < eina_strbuf_length_get(ti->istrdata); i++)
			{
				switch (strdata[i])
				{
					case '>':
						skipflag = 0;
						break;
					case '<':
						skipflag = 1;
						break;
					default:
						if (!skipflag)
							strcnt++;
						break;
				}
				if (strcnt > 100)
					break;
			}
			eina_strbuf_append_n(strent, strdata, i);
			eina_strbuf_replace_all(strent, " absize=240x180 ", " absize=52x39 ");
			if (strcnt > 100)
				eina_strbuf_append(strent, "...");
			elm_entry_scrollable_set(ientry, EINA_TRUE);
			char *entry_text = eina_strbuf_string_get(strent);
			entry_text = _get_string_for_entry(entry_text);
			if (entry_text)
			{
				elm_object_part_text_set(ientry, NULL, entry_text);
				free(entry_text);
			}
			elm_entry_editable_set(ientry, EINA_FALSE);
			elm_entry_context_menu_disabled_set(ientry, EINA_TRUE);
			evas_object_show(ientry);
			elm_layout_content_set(layout, "elm.swallow.inner", ientry);

			eina_strbuf_free(strent);

			return layout;
		}
		else// if (ti->itype == GI_IMAGE)
		{
			Evas_Object *layout = elm_layout_add (obj);
			elm_layout_theme_set(layout, "gengrid", "widestyle", "horizontal_layout");
			edje_object_signal_callback_add(elm_layout_edje_get(layout),
											"mouse,up,1", "*", _grid_item_ly_clicked, data);

			Evas_Object *sicon;
			sicon = evas_object_image_add(evas_object_evas_get(obj));
			evas_object_image_load_size_set(sicon, GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);
			evas_object_image_file_set(sicon, ti->ipathdata, NULL);
			evas_object_image_fill_set(sicon, 0, 0, GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);
			evas_object_resize(sicon, GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);
			elm_layout_content_set(layout, "elm.swallow.icon", sicon);

			struct appdata *ad = g_get_main_appdata();

			if (clipdrawer_paste_textonly_get(ad))
				edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,show,dim", "elm");
			else
				edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,hide,dim", "elm");

			ti->ilayout = layout;
			return layout;
		}
	}

	return NULL;
}

static void _grid_longpress(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
}

static void _grid_click_paste(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
	if (ad->anim_status != STATUS_NONE)
		return;

	Elm_Gengrid_Item *sgobj = NULL;
	sgobj = elm_gengrid_selected_item_get(ad->hig);
	griditem_t *ti = NULL;
	ti = elm_gengrid_item_data_get(sgobj);
}

void _grid_del(const void *data, Evas_Object *obj)
{
	griditem_t *ti = (griditem_t *)data;
	if (ti->itype == GI_TEXT)
		eina_strbuf_free(ti->istrdata);
	else
		eina_stringshare_del(ti->ipathdata);
	free(ti);
}

char* clipdrawer_get_item_data(void *data, int pos)
{
	struct appdata *ad = data;
	griditem_t *ti = NULL;
	griditem_t *newgi = NULL;
	int count = 0;

	if (pos < 0 || pos > ad->hicount)
		return NULL;

	Elm_Gengrid_Item *item = elm_gengrid_first_item_get(ad->hig);
	while (item)
	{
		ti = elm_gengrid_item_data_get(item);
		if (count == pos)
		{
			if (!ti)
				break;
			if (ti->itype == GI_TEXT)
				return (char*)eina_strbuf_string_get(ti->istrdata);
			else
				return ti->ipathdata;
		}
		count++;
		item = elm_gengrid_item_next_get(item);
	}

	return NULL;
}

// FIXME: how to remove calling g_get_main_appdata()?
//        it's mainly used at 'clipdrawer_image_item'
int clipdrawer_add_item(char *idata, int type)
{
	struct appdata *ad = g_get_main_appdata();
	griditem_t *newgi = NULL;

	newgi = malloc(sizeof(griditem_t));
	newgi->itype = type;

	fprintf(stderr, "## add - %d : %s\n", newgi->itype, idata);
	if (type == GI_TEXT)
	{
		newgi->istrdata = eina_strbuf_new();
		eina_strbuf_append(newgi->istrdata, idata);
	}
	else //if (type == GI_IMAGE)
	{
		Elm_Gengrid_Item *item = elm_gengrid_first_item_get(ad->hig);
		griditem_t *ti = NULL;

		if (!check_regular_file(idata))
		{
			DTRACE("Error : it isn't normal file = %s\n", idata);
			return -1;
		}

		while (item)
		{
			ti = elm_gengrid_item_data_get(item);
			if ((ti->itype == type) && !strcmp(ti->ipathdata, idata))
			{
				DTRACE("Error : duplicated file path = %s\n", idata);
				return -1;
			}
			item = elm_gengrid_item_next_get(item);
		}
		newgi->ipathdata = eina_stringshare_add(idata);
	}

	if (ad->hicount >= HISTORY_QUEUE_MAX_ITEMS)
	{
		ad->hicount--;
		// FIXME: add routine that is removing its elements
		elm_gengrid_item_del(elm_gengrid_last_item_get(ad->hig));
	}

	ad->hicount++;
	newgi->item = elm_gengrid_item_prepend(ad->hig, &gic, newgi, NULL, NULL);

	return TRUE;
}

static void
clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	struct appdata *ad = data;

	if (ad->anim_status != STATUS_NONE)
		return;

	#define EDJE_CLOSE_PART_PREFIX "background/close"
	if (!strncmp(source, EDJE_CLOSE_PART_PREFIX, strlen(EDJE_CLOSE_PART_PREFIX)))
	{
		clipdrawer_lower_view(ad);
	}
}

static void set_sliding_win_geometry(void *data)
{
	struct appdata *ad = data;
	Ecore_X_Window zone, xwin;
	Evas_Coord x, y, w, h;
	xwin = elm_win_xwindow_get(ad->win_main);
	zone = ecore_x_e_illume_zone_get(xwin);
	DTRACE("[CBHM] xwin:%x, zone:%x\n", xwin, zone);

//	ecore_evas_geometry_get(ecore_evas_ecore_evas_get(evas_object_evas_get(ad->win_main)), &x, &y, &w, &h);

	if (ad->o_degree == 90 || ad->o_degree == 270)
	{
		h = ad->anim_count * CLIPDRAWER_HEIGHT_LANDSCAPE / ANIM_DURATION;
		x = 0;
		y = ad->root_w - h;
		w = ad->root_h;
	}
	else
	{
		h = ad->anim_count * CLIPDRAWER_HEIGHT / ANIM_DURATION;
		x = 0;
		y = ad->root_h - h;
		w = ad->root_w;
	}

	if (!h)
		w = 0;

	DTRACE("[CBHM] change degree geometry... (%d, %d, %d x %d)\n", x, y, w, h);
	ecore_x_e_illume_sliding_win_geometry_set(zone, x, y, w, h);
	ecore_x_e_illume_sliding_win_state_set(zone, ad->anim_count != 0);
}

void set_rotation_to_clipdrawer(void *data)
{
	struct appdata *ad = data;
	int angle = ad->o_degree;
	int x, y, w, h;

	if (angle == 180) // reverse
	{
		h = CLIPDRAWER_HEIGHT;
		x = 0;
		y = 0;
		w = ad->root_w;
	}
	else if (angle == 90) // right rotate
	{
		h = CLIPDRAWER_HEIGHT_LANDSCAPE;
		x = ad->root_w - h;
		y = 0;
		w = ad->root_h;
	}
	else if (angle == 270) // left rotate
	{
		h = CLIPDRAWER_HEIGHT_LANDSCAPE;
		x = 0;
		y = 0;
		w = ad->root_h;
	}
	else // angle == 0
	{
		h = CLIPDRAWER_HEIGHT;
		x = 0;
		y = ad->root_h - h;
		w = ad->root_w;
	}

	evas_object_resize(ad->win_main, w, h);
	evas_object_move(ad->win_main, x, y);
	if (ad->anim_count == ANIM_DURATION)
		set_sliding_win_geometry(data);
}

int clipdrawer_init(void *data)
{
	struct appdata *ad = data;
	double cdy, cdw;

	ad->windowshow = EINA_FALSE;
	ad->hicount = 0;
	ad->pastetextonly = EINA_TRUE;
	ad->anim_status = STATUS_NONE;
	ad->anim_count = 0;

	// for elm_check
	elm_theme_extension_add(NULL, APP_EDJ_FILE);

	edje_object_signal_callback_add(elm_layout_edje_get(ad->ly_main),
									"mouse,up,1", "*", clipdrawer_ly_clicked, ad);

	ad->hig = NULL;
	ad->hig = elm_gengrid_add(ad->win_main);
	elm_layout_content_set(ad->ly_main, "historyitems", ad->hig);
	elm_gengrid_item_size_set(ad->hig, GRID_ITEM_W+2, GRID_ITEM_H);
	elm_gengrid_align_set(ad->hig, 0.5, 0.5);
	elm_gengrid_horizontal_set(ad->hig, EINA_TRUE);
	elm_gengrid_bounce_set(ad->hig, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(ad->hig, EINA_FALSE);
	evas_object_smart_callback_add(ad->hig, "selected", _grid_click_paste, ad);
//	evas_object_smart_callback_add(ad->hig, "longpressed", _grid_longpress, ad);
	evas_object_size_hint_weight_set(ad->hig, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_gengrid_clear(ad->hig);

	gic.item_style = "default_grid";
	gic.func.label_get = NULL;
	gic.func.content_get = _grid_icon_get;
	gic.func.state_get = NULL;
	gic.func.del = _grid_del;

/*	int i;
	griditem_t *newgi;

	for (i = 0; i < N_IMAGES; i++)
	{
		clipdrawer_add_item(g_images_path[0], GI_IMAGE);
	}

	clipdrawer_add_item("clipboard history", GI_TEXT);*/

	evas_object_show (ad->hig);

// for debugging, calc history pos
/*
   Evas_Coord x, y, w, h;
   Evas_Coord vx, vy, vw, vh;

   edje_object_part_geometry_get(elm_layout_edje_get(ad->ly_main),"imagehistory/list",&x,&y,&w,&h);
   evas_object_geometry_get (ad->hig, &vx,&vy,&vw,&vh);
   fprintf(stderr, "## x = %d, y = %d, w = %d, h = %d\n", x, y, w, h);
   fprintf(stderr, "## vx = %d, vy = %d, vw = %d, vh = %d\n", vx, vy, vw, vh);
*/

//	if (get_item_counts() != 0)
//		clipdrawer_update_contents(ad);

	return 0;
}

int clipdrawer_create_view(void *data)
{
	struct appdata *ad = data;

	clipdrawer_init(ad);

	// for debug
	// at starting, showing app view
	//clipdrawer_activate_view(ad);

	return 0;
}

Eina_Bool _get_anim_pos(void *data, int *sp, int *ep)
{
	if (!sp || !ep)
		return EINA_FALSE;

	struct appdata *ad = data;
	int angle = ad->o_degree;
	int anim_start, anim_end;

	if (angle == 180) // reverse
	{
		anim_start = -(ad->root_h - CLIPDRAWER_HEIGHT);
		anim_end = 0;
	}
	else if (angle == 90) // right rotate
	{
		anim_start = ad->root_w;
		anim_end = anim_start - CLIPDRAWER_HEIGHT_LANDSCAPE;
	}
	else if (angle == 270) // left rotate
	{
		anim_start = -(ad->root_w - CLIPDRAWER_HEIGHT_LANDSCAPE);
		anim_end = 0;
	}
	else // angle == 0
	{
		anim_start = ad->root_h;
		anim_end = anim_start - CLIPDRAWER_HEIGHT;
	}

	*sp = anim_start;
	*ep = anim_end;
	return EINA_TRUE;
}

Eina_Bool _do_anim_delta_pos(void *data, int sp, int ep, int ac, int *dp)
{
	if (!dp)
		return EINA_FALSE;

	struct appdata *ad = data;
	int angle = ad->o_degree;
	int delta;
	double posprop;
	posprop = 1.0*ac/ANIM_DURATION;

	if (angle == 180) // reverse
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(ad->win_main, 0, sp+delta);
	}
	else if (angle == 90) // right rotate
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(ad->win_main, sp+delta, 0);
	}
	else if (angle == 270) // left rotate
	{
		delta = (int)((ep-sp)*posprop);
		evas_object_move(ad->win_main, sp+delta, 0);
	}
	else // angle == 0
	{
		delta = (int)((sp-ep)*posprop);
		evas_object_move(ad->win_main, 0, sp-delta);
	}

	*dp = delta;

	return EINA_TRUE;
}

static void stop_animation(void *data)
{
	struct appdata *ad = data;

	ad->anim_status = STATUS_NONE;
	if (anim_timer)
	{
		ecore_timer_del(anim_timer);
		anim_timer = NULL;
	}

	set_sliding_win_geometry(data);
}

Eina_Bool anim_pos_calc_cb(void *data)
{
	struct appdata *ad = data;

	int anim_start, anim_end, delta;

	_get_anim_pos(ad, &anim_start, &anim_end);

	if (ad->anim_status == SHOW_ANIM)
	{
		if (ad->anim_count > ANIM_DURATION)
		{
			ad->anim_count = ANIM_DURATION;
			stop_animation(data);
			return EINA_FALSE;
		}
		_do_anim_delta_pos(ad, anim_start, anim_end, ad->anim_count, &delta);
		ad->anim_count++;
	}
	else if (ad->anim_status == HIDE_ANIM)
	{
		if (ad->anim_count < 0)
		{
			ad->anim_count = 0;
			evas_object_hide(ad->win_main);
			elm_win_lower(ad->win_main);
			unset_transient_for(ad);
			stop_animation(data);
			return EINA_FALSE;
		}
		_do_anim_delta_pos(ad, anim_start, anim_end, ad->anim_count, &delta);
		ad->anim_count--;
	}
	else
	{
		stop_animation(data);
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

Eina_Bool clipdrawer_anim_effect(void *data, anim_status_t atype)
{
	struct appdata *ad = data;

	if (atype == ad->anim_status)
	{
		DTRACE("Warning: Animation effect is already in progress. \n");
		return EINA_FALSE;
	}

	ad->anim_status = atype;

	if (anim_timer)
		ecore_timer_del(anim_timer);

	anim_timer = ecore_timer_add(ANIM_FLOPS, anim_pos_calc_cb, ad);

	return EINA_TRUE;
}

void clipdrawer_activate_view(void *data)
{
	struct appdata *ad = data;

	if (ad->win_main)
	{
		set_focus_for_app_window(ad->win_main, EINA_TRUE);
		set_transient_for(ad);
		ad->o_degree = get_active_window_degree(ad->active_win);
		elm_win_rotation_set(ad->win_main, ad->o_degree);
		set_rotation_to_clipdrawer(data);
		evas_object_show(ad->win_main);
		elm_win_activate(ad->win_main);
		if (clipdrawer_anim_effect(ad, SHOW_ANIM))
			ad->windowshow = EINA_TRUE;
	}
}

void clipdrawer_lower_view(void *data)
{
	struct appdata *ad = data;

	if (ad->win_main && ad->windowshow)
	{
		set_focus_for_app_window(ad->win_main, EINA_FALSE);
		if (clipdrawer_anim_effect(ad, HIDE_ANIM))
			ad->windowshow = EINA_FALSE;
	}
}

void _change_gengrid_paste_textonly_mode(void *data)
{
	struct appdata *ad = data;

	griditem_t *ti = NULL;

	Elm_Gengrid_Item *item = elm_gengrid_first_item_get(ad->hig);

	while (item)
	{
		ti = elm_gengrid_item_data_get(item);
		if ((ti->itype == GI_IMAGE) && (ti->ilayout))
		{
			if (clipdrawer_paste_textonly_get(ad))
				edje_object_signal_emit(elm_layout_edje_get(ti->ilayout), "elm,state,show,dim", "elm");
			else
				edje_object_signal_emit(elm_layout_edje_get(ti->ilayout), "elm,state,hide,dim", "elm");
		}
		item = elm_gengrid_item_next_get(item);
	}
}

void clipdrawer_paste_textonly_set(void *data, Eina_Bool textonly)
{
	struct appdata *ad = data;
	textonly = !!textonly;
	if (ad->pastetextonly != textonly)
		ad->pastetextonly = textonly;
	DTRACE("paste textonly mode = %d\n", textonly);

	_change_gengrid_paste_textonly_mode(ad);
}

Eina_Bool clipdrawer_paste_textonly_get(void *data)
{
	struct appdata *ad = data;
	return ad->pastetextonly;
}
