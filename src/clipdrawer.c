#include "common.h"
#include "cbhm_main.h"
#include "storage.h"
#include "xcnphandler.h"
#include "clipdrawer.h"

#define IM	"/opt/media/Images and videos/My photo clips/"
static const char *g_images_path[] = {
	IM"1_photo.jpg",
/*
	IM"2_photo.jpg",
	IM"3_photo.jpg",
	IM"4_photo.jpg",
	IM"5_photo.jpg",
	IM"6_photo.jpg",
*/
};
#define N_IMAGES (1)

// gic should live at gengrid callback functions
Elm_Gengrid_Item_Class gic;

typedef struct tag_gridimgitem
{
	Elm_Gengrid_Item *item;
	const char *path;
} gridimgitem_t;

static void _list_click( void *data, Evas_Object *obj, void *event_info )
{
	struct appdata *ad = data;
    Elm_List_Item *it = (Elm_List_Item *) elm_list_selected_item_get(obj);
	if (it == NULL)
		return;

    elm_list_item_selected_set(it, 0);

	char *p = NULL;
	int cplen;

	char *cpdata = NULL;
	cpdata = elm_list_item_label_get(it);
	if (cpdata == NULL)
		return;
	cplen = strlen(cpdata);
	p = malloc(cplen + 1);
	snprintf(p, cplen+1, "%s", cpdata);
	elm_selection_set(1, obj, /*ELM_SEL_FORMAT_TEXT*/1, p);
//	elm_selection_set(1, obj, /*ELM_SEL_FORMAT_MARKUP*/2, p);

//	clipdrawer_lower_view(ad);
}

int clipdrawer_update_contents(void *data)
{
	struct appdata *ad = data;
	int i, pos;

	elm_list_clear(ad->txtlist);
	for (i = 0; i < HISTORY_QUEUE_MAX_TXT_ITEMS; i++)
	{
		pos = get_current_history_position()+i;
		if (pos > HISTORY_QUEUE_MAX_TXT_ITEMS-1)
			pos = pos-HISTORY_QUEUE_MAX_TXT_ITEMS;
		if (get_item_contents_by_pos(pos) != NULL && strlen(get_item_contents_by_pos(pos)) > 0)
		{
			elm_list_item_append(ad->txtlist, get_item_contents_by_pos(pos), NULL, NULL, NULL, ad);
		}
	}
	elm_list_go(ad->txtlist);

	/* FIXME : sometimes when list update, screen isn't updated */

	return 0;
}

const char* clipdrawer_get_plain_string_from_escaped(char *escstr)
{
	/* TODO : is it should be here? besides, remove dependency for entry */
	/* NOTE : return string should be freed */
	return elm_entry_markup_to_utf8(escstr);
}

Evas_Object* grid_icon_get(const void *data, Evas_Object *obj, const char *part)
{
	gridimgitem_t *ti = (gridimgitem_t *)data;
	if (!strcmp(part, "elm.swallow.icon"))
	{
		Evas_Object *icon = elm_icon_add(obj);
		elm_icon_file_set(icon, ti->path, NULL);
		evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
		evas_object_show(icon);
		return icon;
	}
	return NULL;
}

static void grid_selected(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
	char *file, *p;
	int len;
	Elm_Gengrid_Item *sgobj = NULL;
	sgobj = elm_gengrid_selected_item_get(ad->imggrid);
	gridimgitem_t *ti = NULL;
	ti = elm_gengrid_item_data_get(sgobj);

	if (!sgobj || !ti)
	{
		DTRACE("ERR: cbhm can't get the selected image\n");
		return;
	}
	len = strlen(ti->path);
	p = malloc(len + 10);
	snprintf(p,len+10, "file:///%s", ti->path);

	elm_selection_set(/*secondary*/1,obj,/*ELM_SEL_FORMAT_IMAGE*/4,p);

//	clipdrawer_lower_view(ad);

	elm_gengrid_item_selected_set(sgobj, EINA_FALSE);
}

void grid_del(const void *data, Evas_Object *obj)
{
	gridimgitem_t *ti = (gridimgitem_t *)data;
	eina_stringshare_del(ti->path);
	free(ti);
}

// FIXME: how to remove main_ad? 
//        it's mainly used at 'clipdrawer_add_image_item'
int clipdrawer_add_image_item(char *imagepath)
{
	struct appdata *ad = g_get_main_appdata();
	gridimgitem_t *newgenimg = NULL;
	char* filepath = NULL;
	Eina_List *igl = NULL;
	unsigned int igl_counter = 0;
	filepath = &imagepath[7]; // skip 'file://'

	igl = elm_gengrid_items_get(ad->imggrid);
	igl_counter = eina_list_count(igl);
	if (igl_counter >= HISTORY_QUEUE_MAX_IMG_ITEMS)
	{
		elm_gengrid_item_del(eina_list_nth(igl, 0));
	}

	newgenimg = malloc(sizeof(gridimgitem_t));
	newgenimg->path = eina_stringshare_add(filepath);
	newgenimg->item = elm_gengrid_item_append(ad->imggrid, &gic, newgenimg, NULL, NULL);
	
	return TRUE;
}

static void
clipdrawer_ly_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	struct appdata *ad = data;

	#define EDJE_CLOSE_PART_PREFIX "closebutton/"
	if (!strncmp(source, EDJE_CLOSE_PART_PREFIX, strlen(EDJE_CLOSE_PART_PREFIX)))
	{
		clipdrawer_lower_view(ad);
	}
}

int clipdrawer_init(void *data)
{
	struct appdata *ad = data;
	double cdy, cdw;

	cdy = (1.0*CLIPDRAWER_HEIGHT/800)*ad->root_h;
	cdw = (1.0*CLIPDRAWER_POS_Y/800*1.0)*ad->root_h;

	evas_object_resize(ad->win_main, ad->root_w, (int)cdy);
	evas_object_move(ad->win_main, CLIPDRAWER_POS_X, (int)cdw);
	evas_object_resize(ad->ly_main, ad->root_w, (int)cdy);
	evas_object_move(ad->ly_main, CLIPDRAWER_POS_X, (int)cdw);

	edje_object_signal_callback_add(elm_layout_edje_get(ad->ly_main), "mouse,up,1", "*", clipdrawer_ly_clicked, ad);

	ad->imggrid = NULL;
	ad->imggrid = elm_gengrid_add(ad->win_main);
	elm_layout_content_set(ad->ly_main, "imagehistory/list", ad->imggrid);
	elm_gengrid_item_size_set(ad->imggrid, 100, 100+3);
	elm_gengrid_align_set(ad->imggrid, 0.0, 0.0);
	elm_gengrid_horizontal_set(ad->imggrid, EINA_TRUE);
	elm_gengrid_bounce_set(ad->imggrid, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(ad->imggrid, EINA_FALSE);
	evas_object_smart_callback_add(ad->imggrid, "selected", grid_selected, ad);
	evas_object_size_hint_weight_set(ad->imggrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_gengrid_clear(ad->imggrid);

	gic.item_style = "default_grid";
	gic.func.label_get = NULL;
	gic.func.icon_get = grid_icon_get;
	gic.func.state_get = NULL;
	gic.func.del = grid_del;

	int i;
	gridimgitem_t *newgenimg;

	for (i = 0; i < N_IMAGES; i++)
	{
		newgenimg = malloc(sizeof(gridimgitem_t));
		newgenimg->path = eina_stringshare_add(g_images_path[i]);
		newgenimg->item = elm_gengrid_item_append(ad->imggrid, &gic, newgenimg, NULL, NULL);
//		evas_object_data_set(newgenimg->item, "URI", g_images_path[i]);
	}

	evas_object_show (ad->imggrid);

// for debugging, calc image history pos
/*
   Evas_Coord x, y, w, h;
   Evas_Coord vx, vy, vw, vh;

   edje_object_part_geometry_get(elm_layout_edje_get(ad->ly_main),"imagehistory/list",&x,&y,&w,&h);
   evas_object_geometry_get (ad->imggrid, &vx,&vy,&vw,&vh);
   fprintf(stderr, "## x = %d, y = %d, w = %d, h = %d\n", x, y, w, h);
   fprintf(stderr, "## vx = %d, vy = %d, vw = %d, vh = %d\n", vx, vy, vw, vh);
*/

	ad->txtlist = elm_list_add(ad->win_main);
	elm_layout_content_set(ad->ly_main, "texthistory/list", ad->txtlist);
	elm_theme_extension_add(NULL, APP_EDJ_FILE);
	elm_object_style_set(ad->txtlist, "extended/historylist");
	evas_object_size_hint_weight_set(ad->txtlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_smart_callback_add(ad->txtlist, "selected", _list_click, ad);
	elm_list_item_append(ad->txtlist, "default", NULL, NULL, NULL, ad);

	elm_list_go(ad->txtlist);

	if (get_item_counts() != 0)
		clipdrawer_update_contents(ad);

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
	}
}
