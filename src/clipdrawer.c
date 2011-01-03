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
#define GRID_ITEM_SINGLE_W 187
#define GRID_ITEM_SINGLE_H 151
#define GRID_ITEM_W (GRID_ITEM_SINGLE_W+(GRID_ITEM_SPACE_W*2))
#define GRID_ITEM_H (GRID_ITEM_SINGLE_H)
#define GRID_IMAGE_LIMIT_W 91
#define GRID_IMAGE_LIMIT_H 113

// gic should live at gengrid callback functions
Elm_Gengrid_Item_Class gic;

typedef struct tag_griditem
{
	int itype;
	Elm_Gengrid_Item *item;
	const char *ipathdata;
	Eina_Strbuf *istrdata;
	Evas_Object *delbtn;
} griditem_t;

static void _list_click_paste(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
    Elm_List_Item *it = (Elm_List_Item *) elm_list_selected_item_get(obj);
	if (it == NULL)
		return;

    elm_list_item_selected_set(it, 0);

	Elm_List_Item *item;
	Eina_List *n;
	int hc = 0;
	EINA_LIST_FOREACH(elm_list_items_get(ad->txtlist), n, item)
	{
		if (item == it)
			break;
		hc++;
	}

	fprintf(stderr, "## this c = %d, %d\n", hc, get_current_history_position());

	int	pos = get_current_history_position()-hc;
	if (pos < 0)
		pos = pos+(HISTORY_QUEUE_MAX_ITEMS);

	fprintf(stderr, "## pos = %d, %s\n", pos, get_item_contents_by_pos(pos));
	char *p = strdup(get_item_contents_by_pos(pos));

	elm_selection_set(1, obj, /*ELM_SEL_FORMAT_TEXT*/1, p);

/*
	char *p = NULL;
	int cplen;

	char *cpdata = NULL;
	cpdata = elm_entry_utf8_to_markup(elm_list_item_label_get(it));
	if (cpdata == NULL)
		return;
	cplen = strlen(cpdata);
	p = malloc(cplen + 1);
	snprintf(p, cplen+1, "%s", cpdata);
*/
//	elm_selection_set(1, obj, /*ELM_SEL_FORMAT_TEXT*/1, p);
//	elm_selection_set(1, obj, /*ELM_SEL_FORMAT_MARKUP*/2, p);

//	clipdrawer_lower_view(ad);
}

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

		if (get_item_contents_by_pos(pos) != NULL && strlen(get_item_contents_by_pos(pos)) > 0)
		{
			unesc = clipdrawer_get_plain_string_from_escaped(get_item_contents_by_pos(pos));
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
		elm_gengrid_item_del(it);
}

static void _grid_click_delete(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
}

static void
_grid_item_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	struct appdata *ad = g_get_main_appdata();

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
			int len = strlen(ti->ipathdata);
			char *p = malloc(len + 10);
			snprintf(p,len+10, "file:///%s", ti->ipathdata);

			elm_selection_set(/*secondary*/1,obj,/*ELM_SEL_FORMAT_IMAGE*/4,p);
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
			edje_object_signal_callback_add(elm_layout_edje_get(layout), "mouse,up,1", "*", _grid_item_ly_clicked, data);
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

/*
//			edje_object_signal_emit(elm_layout_edje_get(layout), "elm,state,hide,delbtn", "elm");

			Evas_Object *rect = evas_object_rectangle_add(evas_object_evas_get(obj));
//			evas_object_resize(rect, GRID_ITEM_W, GRID_ITEM_H);
			evas_object_color_set(rect, 0, 0, 0, 200);
			evas_object_show(rect);
			elm_layout_content_set(layout, "elm.swallow.cover", rect);
*/

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
	clipdrawer_change_mode(ad);
}

static void _grid_click_paste(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
	Elm_Gengrid_Item *sgobj = NULL;
	sgobj = elm_gengrid_selected_item_get(ad->hig);
	griditem_t *ti = NULL;
	ti = elm_gengrid_item_data_get(sgobj);

	fprintf(stderr, "## grid_click_paste = 0x%x\n", event_info);
}

void _grid_del(const void *data, Evas_Object *obj)
{
	griditem_t *ti = (griditem_t *)data;
	if (ti->itype == GI_IMAGE)
		eina_stringshare_del(ti->ipathdata);
	else
		eina_strbuf_free(ti->istrdata);
	free(ti);
}

// FIXME: how to remove calling g_get_main_appdata()? 
//        it's mainly used at 'clipdrawer_image_item'
int clipdrawer_add_item(char *idata, int type)
{
	struct appdata *ad = g_get_main_appdata();
	griditem_t *newgi = NULL;
	Eina_List *igl = NULL;
	unsigned int igl_counter = 0;

	newgi = malloc(sizeof(griditem_t));
	newgi->itype = type;
	igl = elm_gengrid_items_get(ad->hig);
	igl_counter = eina_list_count(igl);

	fprintf(stderr, "## add - %d : %s\n", newgi->itype, idata);
	if (type == GI_TEXT)
	{
		newgi->istrdata = eina_strbuf_new();
		eina_strbuf_append(newgi->istrdata, idata);
	}
	else //if (type == GI_IMAGE)
	{
		Eina_List *l;
		Elm_Gengrid_Item *item;
		griditem_t *ti = NULL;

		if (!check_regular_file(idata))
		{
			DTRACE("Error : it isn't normal file = %s\n", idata);
			return -1;
		}

		EINA_LIST_FOREACH(igl, l, item)
		{
			ti = elm_gengrid_item_data_get(item);
			if ((ti->itype == type) && !strcmp(ti->ipathdata, idata))
			{
				DTRACE("Error : duplicated file path = %s\n", idata);
				return -1;
			}
		}

		newgi->ipathdata = eina_stringshare_add(idata);
	}

	if (igl_counter >= HISTORY_QUEUE_MAX_ITEMS)
	{
		// FIXME: add routine that is removing its elements
		elm_gengrid_item_del(eina_list_data_get(eina_list_last(igl)));
	}

	newgi->item = elm_gengrid_item_prepend(ad->hig, &gic, newgi, NULL, NULL);

	return TRUE;
}

static void
clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	struct appdata *ad = data;

	#define EDJE_CLOSE_PART_PREFIX "background/close"
	if (!strncmp(source, EDJE_CLOSE_PART_PREFIX, strlen(EDJE_CLOSE_PART_PREFIX)))
	{
		clipdrawer_lower_view(ad);
	}
}

int clipdrawer_init(void *data)
{
	struct appdata *ad = data;
	double cdy, cdw;

	// for elm_check
	elm_theme_extension_add(NULL, APP_EDJ_FILE);

	cdy = (1.0*CLIPDRAWER_HEIGHT/SCREEN_HEIGHT)*ad->root_h;
	cdw = (1.0*CLIPDRAWER_POS_Y/SCREEN_HEIGHT)*ad->root_h;

	evas_object_resize(ad->win_main, ad->root_w, (int)cdy);
	evas_object_move(ad->win_main, CLIPDRAWER_POS_X, (int)cdw);
	evas_object_resize(ad->ly_main, ad->root_w, (int)cdy);
	evas_object_move(ad->ly_main, CLIPDRAWER_POS_X, (int)cdw);

	edje_object_signal_callback_add(elm_layout_edje_get(ad->ly_main), "mouse,up,1", "*", clipdrawer_ly_clicked, ad);

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
	//clipdrawer_add_item("clipboard history asldfjlaskdf las dflkas dflas dfljask dflasd flaksdf jalskdf jalskdf jalsk flaskdfj lkasjf lksad jf", GI_TEXT);

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
	// clipdrawer_activate_view(ad);

	return 0;
}

void clipdrawer_activate_view(void *data)
{
	struct appdata *ad = data;
	
	if (ad->win_main)
	{
		set_transient_for(ad);
		evas_object_show(ad->win_main);
		elm_win_activate(ad->win_main);
	}
}

void clipdrawer_lower_view(void *data)
{
	struct appdata *ad = data;
	
	if (ad->win_main)
	{
		unset_transient_for(ad);
		evas_object_hide(ad->win_main);
		elm_win_lower(ad->win_main);
	}
}
