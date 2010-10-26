#ifndef _cbhm_main_h_
#define _cbhm_main_h_

#include <Elementary.h>

#define APPNAME "Clipboard History Manager"
#define LOCALEDIR "/usr/share/locale"

#define EDJ_PATH "/usr/share/edje"
#define APP_EDJ_FILE EDJ_PATH"/cbhmdrawer.edj"
#define GRP_MAIN "cbhmdrawer"

struct appdata
{
	int root_w;
	int root_h;
	Evas *evas;
	Evas_Object *win_main;
	Evas_Object *ly_main; /* layout widget based on EDJ */
	/* add more variables here */
	Evas_Object *imggrid; 
	Evas_Object *txtlist; 
};

void* g_get_main_appdata();

#endif /* _cbhm_main_h_ */

