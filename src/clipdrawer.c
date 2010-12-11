#include "common.h"
#include "cbhm_main.h"
#include "storage.h"
#include "xcnphandler.h"
#include "clipdrawer.h"

#define DELETE_ICON_PATH "/usr/share/icon/cbhm/05_delete.png"
#define IM	"/usr/share/icon/cbhm/"
static const char *g_images_path[] = {
	IM"cbhm_default_img.png",
/*
	IM"2_photo.jpg",
	IM"3_photo.jpg",
	IM"4_photo.jpg",
	IM"5_photo.jpg",
	IM"6_photo.jpg",
*/
};
#define N_IMAGES (1)

#define GRID_ITEM_W 187
#define GRID_ITEM_H 151
#define GRID_IMAGE_LIMIT_W 91
#define GRID_IMAGE_LIMIT_H 113

// 0 - select mode, 1 - delete mode
static int g_clipdrawer_mode = 0;
// gic should live at gengrid callback functions
Elm_Gengrid_Item_Class gic;

typedef struct tag_griditem
{
	Elm_Gengrid_Item *item;
	const char *idata;
	int itype;
	Evas_Object *delbtn;
} griditem_t;

static int toggle_clipdrawer_mode()
{
	g_clipdrawer_mode = !g_clipdrawer_mode;
	return g_clipdrawer_mode;
}

static int get_clipdrawer_mode()
{
	return g_clipdrawer_mode;
}

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

//	int pos = get_current_history_position() - hc;

	int	pos = get_current_history_position()-hc;
	if (pos < 0)
		pos = pos+(HISTORY_QUEUE_MAX_TXT_ITEMS);

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

	// if delete mode, then back to normal mode
//	if (get_clipdrawer_mode())
//		clipdrawer_change_mode(ad);

	for (i = 0; i < HISTORY_QUEUE_MAX_TXT_ITEMS; i++)
	{
		pos = get_current_history_position() - i;
		if (pos < 0)
			pos = pos+HISTORY_QUEUE_MAX_TXT_ITEMS;

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

Evas_Object* _grid_icon_get(const void *data, Evas_Object *obj, const char *part)
{
	int delete_mode = get_clipdrawer_mode();
	griditem_t *ti = (griditem_t *)data;

	if (!strcmp(part, "elm.swallow.icon"))
	{
		if (ti->itype == GI_TEXT)
		{
			Evas_Object *ientry = elm_entry_add(obj);
			elm_entry_entry_set(ientry, eina_strbuf_string_get(ti->idata));
			elm_entry_background_color_set(ientry, 242, 233, 183, 255);
			elm_entry_editable_set(ientry, EINA_FALSE);
//			evas_object_size_hint_aspect_set(ientry, EVAS_ASPECT_CONTROL_HORIZONTAL, 1, 1);
//			evas_object_resize(ientry, GRID_ITEM_W, GRID_ITEM_H);
			evas_object_show(ientry);
			return ientry;
		}
		else// if (ti->itype == GI_IMAGE)
		{
/*
			Evas_Object *icon = elm_icon_add(obj);
			elm_icon_file_set(icon, ti->idata, NULL);
			evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
			evas_object_show(icon);
*/

			Ecore_Evas *my_ee;
			Evas *my_e;
			Evas_Object *fgimg;
			Evas_Object *bgrect;
			Evas_Object *delbtn;
			Evas_Object *icon;
			my_ee = ecore_evas_buffer_new(GRID_ITEM_W, GRID_ITEM_H);
			my_e = ecore_evas_get(my_ee);

			bgrect = evas_object_rectangle_add(my_e);
			evas_object_color_set(bgrect, 255, 255, 255, 255);
			evas_object_resize(bgrect, GRID_ITEM_W, GRID_ITEM_H);
			evas_object_move(bgrect, 0, 0);
			evas_object_show(bgrect);

#define BORDER_SIZE 1
			fgimg = evas_object_image_add(my_e);
			evas_object_image_load_size_set(fgimg, GRID_ITEM_W-BORDER_SIZE*2, GRID_ITEM_H-BORDER_SIZE*2);
			evas_object_image_file_set(fgimg, ti->idata, NULL);
			evas_object_image_fill_set(fgimg, 0, 0, GRID_ITEM_W-BORDER_SIZE*2, GRID_ITEM_H-BORDER_SIZE*2);
			evas_object_image_filled_set(fgimg, 1);
			int x,y;
			evas_object_image_size_get(fgimg, &x, &y);
			fprintf(stderr, "## img x = %d, y = %d\n", x, y);
			evas_object_resize(fgimg, GRID_ITEM_W-BORDER_SIZE*2, GRID_ITEM_H-BORDER_SIZE*2);
			evas_object_move(fgimg, BORDER_SIZE, BORDER_SIZE);
			evas_object_show(fgimg);

			icon = evas_object_image_add(evas_object_evas_get(obj));

			evas_object_image_data_set(icon, NULL);
			evas_object_image_size_set(icon, GRID_ITEM_W, GRID_ITEM_H);
			evas_object_image_fill_set(icon, 0, 0, GRID_ITEM_W, GRID_ITEM_H);
			evas_object_image_filled_set(icon, EINA_TRUE);
			evas_object_image_data_copy_set(icon, (int *)ecore_evas_buffer_pixels_get(my_ee));
			evas_object_image_data_update_add(icon, 0, 0, GRID_ITEM_W, GRID_ITEM_H);

			evas_object_del(bgrect);
			evas_object_del(fgimg);
			ecore_evas_free(my_ee);

			return icon;
		}

//		return icon;
	}
/*
	else if (!strcmp(part, "elm.swallow.end") && delete_mode)
	{
		ti->delbtn = elm_check_add(obj);
		elm_object_style_set(ti->delbtn, "extended/imagegrid");
		elm_check_state_set(ti->delbtn, EINA_TRUE);
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
	char *file, *p;
	int len;
	Elm_Gengrid_Item *sgobj = NULL;
	sgobj = elm_gengrid_selected_item_get(ad->hig);
	griditem_t *ti = NULL;
	ti = elm_gengrid_item_data_get(sgobj);

	if (!sgobj || !ti)
	{
		DTRACE("ERR: cbhm can't get the selected image\n");
		return;
	}
	if (ti->itype == GI_TEXT)
	{
		char *p = strdup(eina_strbuf_string_get(ti->idata));

		elm_selection_set(1, obj, /*ELM_SEL_FORMAT_TEXT*/1, p);
	}
	else //if (ti->itype == GI_IMAGE)
	{
		len = strlen(ti->idata);
		p = malloc(len + 10);
		snprintf(p,len+10, "file:///%s", ti->idata);

		elm_selection_set(/*secondary*/1,obj,/*ELM_SEL_FORMAT_IMAGE*/4,p);

		elm_gengrid_item_selected_set(sgobj, EINA_FALSE);
	}
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
	Elm_Gengrid_Item *sgobj = NULL;
	sgobj = elm_gengrid_selected_item_get(ad->hig);
	griditem_t *ti = NULL;
	ti = elm_gengrid_item_data_get(sgobj);

	if (!sgobj || !ti)
	{
		DTRACE("ERR: cbhm can't get the clicked del image\n");
		return;
	}

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

void _grid_del(const void *data, Evas_Object *obj)
{
	griditem_t *ti = (griditem_t *)data;
	if (ti->itype == GI_IMAGE)
		eina_stringshare_del(ti->idata);
	else
		eina_strbuf_free(ti->idata);
	free(ti);
}

int clipdrawer_refresh_history_item(void *data, int delete_mode)
{
	struct appdata *ad = data;
	Eina_List *oldlist = NULL;
	const Eina_List *l;
	Elm_Gengrid_Item *lgrid;
	griditem_t *lgitem;
	Evas_Object *ngg;
	Evas_Object *oldgg;
	
	oldlist = elm_gengrid_items_get(ad->hig);
	elm_layout_content_unset(ad->ly_main, "imagehistory/list");
	ngg = elm_gengrid_add(ad->win_main);
	elm_layout_content_set(ad->ly_main, "imagehistory/list", ngg);
	oldgg = ad->hig;
	ad->hig = ngg;
	elm_gengrid_item_size_set(ad->hig, GRID_ITEM_W, GRID_ITEM_H+3);
	elm_gengrid_align_set(ad->hig, 0.5, 0.5);
//	elm_gengrid_horizontal_set(ad->hig, EINA_TRUE);
	elm_gengrid_bounce_set(ad->hig, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(ad->hig, EINA_FALSE);
	if (delete_mode)
		evas_object_smart_callback_add(ad->hig, "selected", _grid_click_delete, ad);
	else
		evas_object_smart_callback_add(ad->hig, "selected", _grid_click_paste, ad);
	evas_object_smart_callback_add(ad->hig, "longpressed", _grid_longpress, ad);
	evas_object_size_hint_weight_set(ad->hig, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	gic.item_style = "default_grid";
	gic.func.label_get = NULL;
	gic.func.icon_get = _grid_icon_get;
	gic.func.state_get = NULL;
	gic.func.del = _grid_del;

	EINA_LIST_REVERSE_FOREACH(oldlist, l, lgrid)
	{
		lgitem = elm_gengrid_item_data_get(lgrid);
		clipdrawer_add_item(lgitem->idata, GI_IMAGE);
	}

	evas_object_show (ad->hig);

	elm_gengrid_clear(oldgg);
	evas_object_hide(oldgg);
	evas_object_del(oldgg);

	return 0;
}

int clipdrawer_change_mode(void *data)
{
	struct appdata *ad = data;

	toggle_clipdrawer_mode();

	DTRACE("clipdrawer delete mode = %d\n", get_clipdrawer_mode());

	clipdrawer_refresh_history_item(ad, get_clipdrawer_mode());

	return 0;
}

// FIXME: how to remove calling g_get_main_appdata()? 
//        it's mainly used at 'clipdrawer_image_item'
int clipdrawer_add_item(char *idata, int type)
{
	struct appdata *ad = g_get_main_appdata();
	griditem_t *newgi = NULL;
	char* filepath = NULL;
	Eina_List *igl = NULL;
	unsigned int igl_counter = 0;

	// if delete mode, then back to normal mode
//	if (get_clipdrawer_mode())
//		clipdrawer_change_mode(ad);

/*
	if (!check_regular_file(imagepath))
	{
		DTRACE("Error : it isn't normal file = %s\n", imagepath);
		return -1;
	}

	igl = elm_gengrid_items_get(ad->hig);
	igl_counter = eina_list_count(igl);

	Eina_List *l;
	Elm_Gengrid_Item *item;
	griditem_t *ti = NULL;

	EINA_LIST_FOREACH(igl, l, item)
	{
		ti = elm_gengrid_item_data_get(item);
		if (!strcmp(ti->path, imagepath))
		{
			DTRACE("Error : duplicated file path = %s\n", imagepath);
			return -1;
		}
	}

	if (igl_counter >= HISTORY_QUEUE_MAX_IMG_ITEMS)
	{
		elm_gengrid_item_del(eina_list_data_get(eina_list_last(igl)));
	}

	newgi = malloc(sizeof(griditem_t));
	newgi->itype = GI_IMAGE;
	newgi->idata = eina_stringshare_add(imagepath);
	newgi->item = elm_gengrid_item_prepend(ad->hig, &gic, newgi, NULL, NULL);
*/

/*
	static int testmode = 0;
	testmode++;

	newgi = malloc(sizeof(griditem_t));
	if (testmode % 3)
	{
	newgi->itype = GI_IMAGE;
	newgi->idata = eina_stringshare_add(imagepath);
	}
	else
	{
	newgi->itype = GI_TEXT;
	newgi->idata = eina_strbuf_new();
	eina_strbuf_append(newgi->idata, "hello!! <item absize=40x30 href=file:///usr/share/icon/cbhm/cbhm_default_img.png></item>");

	}
	newgi->item = elm_gengrid_item_append(ad->hig, &gic, newgi, NULL, NULL);
*/

	newgi = malloc(sizeof(griditem_t));
	newgi->itype = type;

	fprintf(stderr, "## add %d : %s\n", newgi->itype, idata);
	if (type == GI_TEXT)
	{
		newgi->idata = eina_strbuf_new();
		eina_strbuf_append_n(newgi->idata, idata, 69);
		if (strlen(idata) > 69)
			eina_strbuf_append(newgi->idata, "...");
	}
	else //if (type == GI_IMAGE)
	{
		newgi->idata = eina_stringshare_add(idata);
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
	elm_gengrid_item_size_set(ad->hig, GRID_ITEM_W+3, GRID_ITEM_H);
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

/*
	clipdrawer_add_item("some aslkdfjalskdjflkasdf as dflkjas df aslk fjalskdf jlaks djflaksj dflkas flkas jdflkas jdflkasj dflk asjldfk jqwlkerj qlkw jflkzjx cvlkzx vlkasj fldkjqwlkerj qwlkerj qwlrkj asldkfjalskdjflaskdjflaskjdflaksjdflkasjdflkjasldfkjaslkrj123i4o uosadf osapd fuoasiuer lqw rlqwkj foiasudfqlkj;lrqjewlr  ", GI_TEXT);

	clipdrawer_add_item("appliance r  ", GI_TEXT);
	clipdrawer_add_item("appliance k  ", GI_TEXT);
	clipdrawer_add_item("appliance s  ", GI_TEXT);
*/

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
	clipdrawer_activate_view(ad);

	return 0;
}

void clipdrawer_activate_view(void *data)
{
	struct appdata *ad = data;
	
	if (ad->win_main)
	{
		evas_object_show(ad->win_main);
		elm_win_activate(ad->win_main);
	}
}

void clipdrawer_lower_view(void *data)
{
	struct appdata *ad = data;
	
	if (ad->win_main)
	{
		evas_object_hide(ad->win_main);
		elm_win_lower(ad->win_main);

		// if delete mode, then back to normal mode
		if (get_clipdrawer_mode())
			clipdrawer_change_mode(ad);
	}
}
