#ifndef _cbhm_main_h_
#define _cbhm_main_h_

#include <Elementary.h>

#define APPNAME "Clipboard History Manager"
#define LOCALEDIR "/usr/share/locale"

#define EDJ_PATH "/usr/share/edje"
#define EDJ_FILE "/usr/share/edje/cbhmdrawer.edj"
#define GRP_MAIN "cbhmdrawer"

struct appdata
{
	Evas *evas;
	Evas_Object *win_main;
	Evas_Object *ly_main; /* layout widget based on EDJ */
	/* add more variables here */
	Evas_Object *scrl;
	Evas_Object *imgbox; 
	Evas_Object *txtlist; 
};

#endif /* _cbhm_main_h_ */

