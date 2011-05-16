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
Ecore_Timer *tm_anim;

typedef struct tag_griditem
{
	int itype;
	Elm_Gengrid_Item *item;
	const char *ipathdata;
	Eina_Strbuf *istrdata;
	Evas_Object *delbtn;
	Evas_Object *ilayout;
} griditem_t;


int clipdrawer_update_contents(void *data)
{
	struct appdata *ad = data;
	int i, pos;
	char *unesc = NULL;

	for (i = 0; i < HISTORY_QUEUE_MAX_ITEMS; i++)
	{
		pos = get_current_history_position() - i;
		if (pos < 0)
			pos = pos+HISTORY_QUEUE_MAX_ITEMS;

		if (clipdrawer_get_item_data(ad, pos) != NULL && strlen(clipdrawer_get_item_data(ad, pos)) > 0)
		{
			unesc = clipdrawer_get_plain_string_from_escaped(clipdrawer_get_item_data(ad, pos));
			unesc = unesc ? unesc : "";
			elm_list_item_append(ad->txtlist, unesc, NULL, NULL, NULL, ad);
			free(unesc);
		}
	}

	/* FIXME : sometimes when list update, screen isn't updated */

	return 0;
}

const char* clipdrawer_get_plain_string_from_escaped(char *escstr)
{
	/* TODO : is it should be here? besides, remove dependency for entry */
	/* NOTE : return string should be freed */
	return elm_entry_markup_to_utf8(escstr);
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

			elm_selection_set(1, obj, /*ELM_SEL_FORMAT_TEXT*/1, p);
		}
		else //if (ti->itype == GI_IMAGE)
		{
			if (!clipdrawer_paste_textonly_get(ad))
			{
				int len = strlen(ti->ipathdata);
				char *p = malloc(len + 10);
				snprintf(p,len+10, "file:///%s", ti->ipathdata);

				elm_selection_set(/*secondary*/1,obj,/*ELM_SEL_FORMAT_IMAGE*/4,p);
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
			Evas_Object *ientry = elm_scrolled_entry_add(obj);
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
//			eina_strbuf_append(strent, strdata);
			eina_strbuf_replace_all(strent, " absize=240x180 ", " absize=52x39 ");
			if (strcnt > 100)
				eina_strbuf_append(strent, "...");
			eina_strbuf_prepend(strent, "<font_size=18>");

			elm_scrolled_entry_entry_set(ientry, eina_strbuf_string_get(strent));
			elm_scrolled_entry_editable_set(ientry, EINA_FALSE);
			elm_scrolled_entry_context_menu_disabled_set(ientry, EINA_TRUE);
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
			
			if (!clipdrawer_paste_textonly_get(ad))
			{
				edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,hide,delbtn", "elm");
				Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(obj));
				evas_object_color_set(rect, 0, 0, 0, 200);
				evas_object_show(rect);
				elm_layout_content_set(layout, "elm.swallow.cover", rect);
			}

			ti->ilayout = layout;
			return layout;

/*
			Evas_Object *icon = elm_icon_add(obj);
			elm_icon_file_set(icon, ti->ipathdata, NULL);
			evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
			evas_object_show(icon);
*/

/*
			Ecore_Evas *my_ee;
			Evas *my_e;
			Evas_Object *fgimg;
			Evas_Object *bgrect;
			Evas_Object *delbtn;
			Evas_Object *icon;
			my_ee = ecore_evas_buffer_new(GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);
			my_e = ecore_evas_get(my_ee);

			bgrect = evas_object_rectangle_add(my_e);
			evas_object_color_set(bgrect, 255, 255, 255, 255);
			evas_object_resize(bgrect, GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);
			evas_object_move(bgrect, 0, 0);
			evas_object_show(bgrect);

#define BORDER_SIZE 1
			fgimg = evas_object_image_add(my_e);
			evas_object_image_load_size_set(fgimg, GRID_ITEM_SINGLE_W-BORDER_SIZE*2, GRID_ITEM_SINGLE_H-BORDER_SIZE*2);
			evas_object_image_file_set(fgimg, ti->ipathdata, NULL);
			evas_object_image_fill_set(fgimg, 0, 0, GRID_ITEM_SINGLE_W-BORDER_SIZE*2, GRID_ITEM_SINGLE_H-BORDER_SIZE*2);
			evas_object_image_filled_set(fgimg, 1);
			int x,y;
			evas_object_image_size_get(fgimg, &x, &y);
			//fprintf(stderr, "## img x = %d, y = %d\n", x, y);
			evas_object_resize(fgimg, GRID_ITEM_SINGLE_W-BORDER_SIZE*2, GRID_ITEM_SINGLE_H-BORDER_SIZE*2);
			evas_object_move(fgimg, BORDER_SIZE, BORDER_SIZE);
			evas_object_show(fgimg);

			icon = evas_object_image_add(evas_object_evas_get(obj));

			evas_object_image_data_set(icon, NULL);
			evas_object_image_size_set(icon, GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);
			evas_object_image_fill_set(icon, 0, 0, GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);
			evas_object_image_filled_set(icon, EINA_TRUE);
			evas_object_image_data_copy_set(icon, (int *)ecore_evas_buffer_pixels_get(my_ee));
			evas_object_image_data_update_add(icon, 0, 0, GRID_ITEM_SINGLE_W, GRID_ITEM_SINGLE_H);

			evas_object_del(bgrect);
			evas_object_del(fgimg);
			ecore_evas_free(my_ee);

			return icon;
*/
		}

//		return icon;
	}
/*
	else if (!strcmp(part, "elm.swallow.end"))
	{
		ti->delbtn = elm_check_add(obj);
		elm_object_style_set(ti->delbtn, "extended/itemcheck");
		//evas_object_propagate_events_set(ti->delbtn, 0);
		elm_check_state_set(ti->delbtn, tcm);
		evas_object_smart_callback_add(ti->delbtn, "changed", _grid_item_check_changed, data);
		evas_object_show(ti->delbtn);
		return ti->delbtn;
	}
*/
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

void set_rotation_to_clipdrawer(void *data, int angle)
{
	struct appdata *ad = data;
	double wh, wy;
	int wposx, wwidth;

	if (angle == 180) // reverse
	{
		wh = (1.0*CLIPDRAWER_HEIGHT/SCREEN_HEIGHT)*ad->root_h;
		wy = 0;
		wwidth = ad->root_w;
		wposx = CLIPDRAWER_POS_X;
	}
	else if (angle == 90) // right rotate
	{
		wh = (1.0*CLIPDRAWER_HEIGHT_LANDSCAPE/SCREEN_WIDTH)*ad->root_w;
		wy = (1.0*CLIPDRAWER_POS_X/SCREEN_WIDTH)*ad->root_w;
		wwidth = ad->root_h;
		wposx = CLIPDRAWER_WIDTH-CLIPDRAWER_HEIGHT_LANDSCAPE;
	}
	else if (angle == -90) // left rotate
	{
		wh = (1.0*CLIPDRAWER_HEIGHT_LANDSCAPE/SCREEN_WIDTH)*ad->root_w;
		wy = (1.0*CLIPDRAWER_POS_X/SCREEN_WIDTH)*ad->root_w;
		wwidth = ad->root_h;
		wposx = CLIPDRAWER_POS_X;
	}
	else // angle == 0
	{
		wh = (1.0*CLIPDRAWER_HEIGHT/SCREEN_HEIGHT)*ad->root_h;
		wy = (1.0*CLIPDRAWER_POS_Y/SCREEN_HEIGHT)*ad->root_h;
		wwidth = ad->root_w;
		wposx = CLIPDRAWER_POS_X;
 	}

	evas_object_resize(ad->win_main, wwidth, (int)wh);
	evas_object_move(ad->win_main, wposx, (int)wy);
	evas_object_resize(ad->ly_main, wwidth, (int)wh);
	evas_object_move(ad->ly_main, wposx, (int)wy);
}

int clipdrawer_init(void *data)
{
	struct appdata *ad = data;
	double cdy, cdw;

	ad->windowshow = EINA_FALSE;
	ad->hicount = 0;
	ad->pastetextonly = EINA_TRUE;
	ad->anim_status = STATUS_NONE;

	// for elm_check
	elm_theme_extension_add(NULL, APP_EDJ_FILE);

	set_rotation_to_clipdrawer(ad, get_rotation_degree());

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
	gic.func.icon_get = _grid_icon_get;
	gic.func.state_get = NULL;
	gic.func.del = _grid_del;

	int i;
	griditem_t *newgi;

	for (i = 0; i < N_IMAGES; i++)
	{
		clipdrawer_add_item(g_images_path[0], GI_IMAGE);
	}

	clipdrawer_add_item("clipboard history", GI_TEXT);

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
	int angle = get_rotation_degree();
	int anim_start, anim_end, delta;

	switch (ad->anim_status)
	{
		case HIDE_ANIM:
			if (angle == 180) // reverse
			{
				anim_start = 0;
				anim_end = (int)(((double)CLIPDRAWER_HEIGHT/SCREEN_HEIGHT)*ad->root_h);
				anim_end = ad->root_h-anim_end;
				anim_end = -anim_end;
			}
			else if (angle == 90) // right rotate
			{
				anim_end = ad->root_w;
				anim_start = (int)(((double)CLIPDRAWER_HEIGHT_LANDSCAPE/SCREEN_WIDTH)*ad->root_w);
				anim_start = ad->root_w-anim_start;
			}
			else if (angle == -90) // left rotate
			{
				anim_start = 0;
				anim_end = (int)(((double)CLIPDRAWER_HEIGHT_LANDSCAPE/SCREEN_WIDTH)*ad->root_w);
				anim_end = ad->root_w-anim_end;
				anim_end = -anim_end;
			}
			else // angle == 0
			{
				anim_start = (int)((1.0*CLIPDRAWER_HEIGHT/SCREEN_HEIGHT)*ad->root_h);
				anim_end = ad->root_h;
				anim_start = anim_end-anim_start;
			}
			break;
		case SHOW_ANIM:
			if (angle == 180) // reverse
			{
				anim_start = (int)(((double)CLIPDRAWER_HEIGHT/SCREEN_HEIGHT)*ad->root_h);
				anim_start = ad->root_h - anim_start;
				anim_start = -anim_start;
				anim_end = 0;
			}
			else if (angle == 90) // right rotate
			{
				anim_start = ad->root_w;
				anim_end = (int)(((double)CLIPDRAWER_HEIGHT_LANDSCAPE/SCREEN_WIDTH)*ad->root_w);
				anim_end = anim_start-anim_end;
			}
			else if (angle == -90) // left rotate
			{
				anim_start = (int)(((double)CLIPDRAWER_HEIGHT_LANDSCAPE/SCREEN_WIDTH)*ad->root_w);
				anim_start = ad->root_w-anim_start;
				anim_start = -anim_start;
				anim_end = 0;
			}
			else // angle == 0
			{
				anim_start = ad->root_h;
				anim_end = (int)(((double)CLIPDRAWER_HEIGHT/SCREEN_HEIGHT)*ad->root_h);
				anim_end = anim_start-anim_end;
			}
			break;
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
	int angle = get_rotation_degree();
	int delta;
	double posprop;
	posprop = 1.0*ac/ANIM_DURATION;

	switch (ad->anim_status)
	{
		case HIDE_ANIM:
			if (angle == 180) // reverse
			{
				delta = (int)((sp-ep)*posprop);
				evas_object_move(ad->win_main, 0, sp-delta);
			}
			else if (angle == 90) // right rotate
			{
				delta = (int)((sp-ep)*posprop);
				evas_object_move(ad->win_main, sp-delta, 0);
			}
			else if (angle == -90) // left rotate
			{
				delta = (int)((sp-ep)*posprop);
				evas_object_move(ad->win_main, sp-delta, 0);
			}
			else // angle == 0
			{
				delta = (int)((ep-sp)*posprop);
				evas_object_move(ad->win_main, 0, sp+delta);
			}
			break;
		case SHOW_ANIM:
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
			else if (angle == -90) // left rotate
			{
				delta = (int)((ep-sp)*posprop);
				evas_object_move(ad->win_main, sp+delta, 0);
			}
			else // angle == 0
			{
				delta = (int)((sp-ep)*posprop);
				evas_object_move(ad->win_main, 0, sp-delta);
			}
			break;
	}
	
	*dp = delta;

	return EINA_TRUE;
}

Eina_Bool anim_pos_calc_cb(void *data)
{
	struct appdata *ad = data;

	static int anim_count = 0;
	int anim_start, anim_end, delta;

	_get_anim_pos(ad, &anim_start, &anim_end);

	if (anim_count > ANIM_DURATION)
	{
		anim_count = 0;
		if (ad->anim_status == HIDE_ANIM)
		{
			evas_object_hide(ad->win_main);
			elm_win_lower(ad->win_main);
			unset_transient_for(ad);
		}
		ad->anim_status = STATUS_NONE;
		set_focus_for_app_window(ad->win_main, EINA_FALSE);
		return EINA_FALSE;
	}

	_do_anim_delta_pos(ad, anim_start, anim_end, anim_count, &delta);

	anim_count++;
	return EINA_TRUE;
}

void clipdrawer_anim_effect(void *data, anim_status_t atype)
{
	struct appdata *ad = data;

	if (ad->anim_status != STATUS_NONE)
	{
		DTRACE("ERR: another animation is showing\n");
		return;
	}

	ad->anim_status = atype;
	ecore_timer_add(ANIM_FLOPS, anim_pos_calc_cb, ad);
}

void clipdrawer_activate_view(void *data)
{
	struct appdata *ad = data;

	set_focus_for_app_window(ad->win_main, EINA_TRUE);
	if (ad->win_main)
	{
		set_transient_for(ad);
		evas_object_show(ad->win_main);
		elm_win_activate(ad->win_main);
//		elm_win_raise(ad->win_main);
		clipdrawer_anim_effect(ad, SHOW_ANIM);
		ad->windowshow = EINA_TRUE;
	}
}

void clipdrawer_lower_view(void *data)
{
	struct appdata *ad = data;
	
	if (ad->win_main && ad->windowshow)
	{
		clipdrawer_anim_effect(ad, HIDE_ANIM);
		ad->windowshow = EINA_FALSE;
	}
}

void _change_gengrid_paste_textonly_mode(void *data)
{
	struct appdata *ad = data;

	Elm_Gengrid_Item *item;
	griditem_t *ti = NULL;

	if (clipdrawer_paste_textonly_get(ad))
	{ // textonly paste mode
		Elm_Gengrid_Item *item = elm_gengrid_first_item_get(ad->hig);

		while (item)	
		{
			ti = elm_gengrid_item_data_get(item);
			if ((ti->itype == GI_IMAGE) && (ti->ilayout))
			{
				edje_object_signal_emit(elm_layout_edje_get(ti->ilayout), "elm,state,hide,delbtn", "elm");
				Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(ad->hig));
				evas_object_color_set(rect, 0, 0, 0, 200);
				evas_object_show(rect);
				elm_layout_content_set(ti->ilayout, "elm.swallow.cover", rect);
			}
			item = elm_gengrid_item_next_get(item);
		}
	}
	else
	{ // text+image paste mode
		Elm_Gengrid_Item *item = elm_gengrid_first_item_get(ad->hig);

		while (item)	
		{
			ti = elm_gengrid_item_data_get(item);
			if ((ti->itype == GI_IMAGE) && (ti->ilayout))
			{
				edje_object_signal_emit(elm_layout_edje_get(ti->ilayout), "elm,state,show,delbtn", "elm");
				Evas_Object *rect = elm_layout_content_unset(ti->ilayout, "elm.swallow.cover");
				evas_object_hide(rect);
				evas_object_del(rect);
			}
			item = elm_gengrid_item_next_get(item);	     
		}
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
