#include "common.h"
#include "clipdrawer.h"
#include "cbhm_main.h"
#include "storage.h"

#ifndef _EDJ
#define _EDJ(ly) elm_layout_edje_get(ly)
#endif

#define IM	"/mnt/ums/Images/Wallpapers/"
static const char *images[] = {
	IM"TFT_01_480x800.jpg",
	IM"TFT_02_480x800.jpg",
	IM"TFT_03_480x800.jpg",
	IM"TFT_04_480x800.jpg",
	IM"TFT_05_480x800.jpg",
	IM"OCTA_01_480x800.jpg",
	IM"OCTA_02_480x800.jpg",
	IM"OCTA_03_480x800.jpg",
	IM"OCTA_04_480x800.jpg",
	IM"OCTA_05_480x800.jpg",
};
#define N_IMAGES (10)

static void _list_click( void *data, Evas_Object *obj, void *event_info )
{
    Elm_List_Item *it = (Elm_List_Item *) elm_list_selected_item_get( obj );
	if (it == NULL)
		return;

    elm_list_item_selected_set(it, 0);

	char *p = NULL;
	char *cpdata = NULL;
	int clen;

	cpdata = elm_list_item_label_get(it);
	if (cpdata == NULL)
		return;
	clen = strlen(cpdata);
	p = malloc(clen + 1);
	snprintf(p, clen, "%s", cpdata);
	fprintf(stderr, "## cbhm : sel : %s\n", p);
	elm_selection_set(1, obj, /*mark up*/1, p);
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
		if (get_item_contents_by_pos(pos) != NULL)
			elm_list_item_append(ad->txtlist, get_item_contents_by_pos(pos), NULL, NULL, NULL, ad);
	}
	elm_list_go(ad->txtlist);

	return 0;
}

static int clipdrawer_init(void *data)
{
	struct appdata *ad = data;

	evas_object_resize(ad->win_main, 480, 400);
	evas_object_move(ad->win_main, 0, 400);
	evas_object_resize(ad->ly_main, 480, 400);
	evas_object_move(ad->ly_main, 0, 400);

	ad->scrl = elm_scroller_add(ad->win_main);
	edje_object_part_swallow(_EDJ(ad->ly_main), "cbhmdrawer/imglist", ad->scrl);
	evas_object_size_hint_weight_set(ad->scrl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_scroller_bounce_set(ad->scrl, EINA_TRUE, EINA_FALSE);
	elm_scroller_policy_set(ad->scrl, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
	elm_win_resize_object_add(ad->win_main, ad->scrl);
	evas_object_show(ad->scrl);

	evas_object_resize(ad->scrl,480,95);
 
	ad->imgbox = elm_box_add(ad->win_main);
	elm_box_horizontal_set(ad->imgbox, TRUE);
	evas_object_size_hint_weight_set(ad->imgbox, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_align_set(ad->imgbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_content_set(ad->scrl, ad->imgbox);
	evas_object_show(ad->imgbox);

	Evas_Object *pt;
	int i;
	for (i = 0 ; i < N_IMAGES ; i ++)
	{
		pt = elm_photo_add(ad->win_main);
		elm_photo_file_set(pt, images[i]);
		evas_object_size_hint_weight_set(pt, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(pt, EVAS_HINT_FILL,
				EVAS_HINT_FILL);
		elm_photo_size_set(pt, 145);
		elm_box_pack_end(ad->imgbox, pt);
		evas_object_show(pt);
		evas_object_data_set(pt,"URI",images[i]);
	}

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

	return 0;
}

void clipdrawer_activate_view(void *data)
{
	struct appdata *ad = data;
	
	if (ad->win_main)
		elm_win_activate(ad->win_main);
}
