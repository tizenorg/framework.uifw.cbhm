#include "common.h"
#include "cbhm_main.h"
#include "storage.h"
#include "xcnphandler.h"
#include "clipdrawer.h"

#define IM	"/mnt/ums/Images/Photo/"
static const char *g_images_path[] = {
	IM"1_photo.jpg",
	IM"2_photo.jpg",
	IM"3_photo.jpg",
	IM"4_photo.jpg",
	IM"5_photo.jpg",
	IM"6_photo.jpg",
};
#define N_IMAGES (6)

typedef struct tag_gridimgitem
{
	Elm_Gengrid_Item *item;
	const char *path;
} gridimgitem_t;

static void
_image_click(void *data, Evas_Object *obj, void *event_info)
{
	struct appdata *ad = data;
	char *p,*file;
	int len;
	file = evas_object_data_get(obj, "URI");
	len = strlen(file);
	p = malloc(len + 10);
	snprintf(p,len+10,"file:///%s",file);

	elm_selection_set(/*secondary*/1,obj,/*ELM_SEL_FORMAT_IMAGE*/4,p);

	clipdrawer_lower_view(ad);
}

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

	clipdrawer_lower_view(ad);
}

int clipdrawer_update_contents(void *data)
{
	struct appdata *ad = data;
	int i, pos;

	elm_list_clear(ad->txtlist);
	for (i = 0; i < HISTORY_QUEUE_NUMBER; i++)
	{
		pos = get_current_history_position()+i;
		if (pos > HISTORY_QUEUE_NUMBER-1)
			pos = pos-HISTORY_QUEUE_NUMBER;
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

	clipdrawer_lower_view(ad);

	elm_gengrid_item_selected_set(sgobj, EINA_FALSE);
}

void grid_del(const void *data, Evas_Object *obj)
{
	gridimgitem_t *ti = (gridimgitem_t *)data;
	eina_stringshare_del(ti->path);
	free(ti);
}

int clipdrawer_init(void *data)
{
	struct appdata *ad = data;

	evas_object_resize(ad->win_main, 480, 360);
	evas_object_move(ad->win_main, 0, 440);
	evas_object_resize(ad->ly_main, 480, 360);
	evas_object_move(ad->ly_main, 0, 440);

	ad->imggrid = NULL;
	ad->imggrid = elm_gengrid_add(ad->win_main);
	elm_layout_content_set(ad->ly_main, "cbhmdrawer/imglist", ad->imggrid);
	elm_gengrid_item_size_set(ad->imggrid, 125, 135);
	elm_gengrid_align_set(ad->imggrid, 0.5, 0.0);
	elm_gengrid_horizontal_set(ad->imggrid, EINA_TRUE);
	elm_gengrid_bounce_set(ad->imggrid, EINA_TRUE, EINA_FALSE);
	elm_gengrid_multi_select_set(ad->imggrid, EINA_FALSE);
	evas_object_smart_callback_add(ad->imggrid, "selected", grid_selected, ad);
	evas_object_size_hint_weight_set(ad->imggrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_gengrid_clear(ad->imggrid);

	// gic should live at gengrid callback functions
	static Elm_Gengrid_Item_Class gic;
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

	ad->txtlist = elm_list_add(ad->win_main);
	elm_layout_content_set(ad->ly_main, "cbhmdrawer/txtlist", ad->txtlist);
	elm_list_horizontal_mode_set(ad->txtlist, ELM_LIST_COMPRESS);
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
