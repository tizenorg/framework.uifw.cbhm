#ifndef _clipdrawer_h_
#define _clipdrawer_h_

#define CLIPDRAWER_HEIGHT 360
#define CLIPDRAWER_HEIGHT_LANDSCAPE 228

enum {
	GI_TEXT = 0,
	GI_IMAGE,

	GI_MAX_ITEMS,
};

/* view maintains */
int clipdrawer_init(void *data);
int clipdrawer_create_view(void *data);
void clipdrawer_activate_view(void *data);
//void clipdrawer_hide_view(void *data);
void clipdrawer_lower_view(void *data);

void set_rotation_to_clipdrawer(void *data);

const char* clipdrawer_get_plain_string_from_escaped(char *escstr);

char *clipdrawer_get_item_data(void *data, int pos);
int clipdrawer_add_item(char *idata, int type);

void clipdrawer_paste_textonly_set(void *data, Eina_Bool textonly);
Eina_Bool clipdrawer_paste_textonly_get(void *data);

#endif // _clipdrawer_h_
