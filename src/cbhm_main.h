#ifndef _cbhm_main_h_
#define _cbhm_main_h_

#include <Elementary.h>

#if !defined(PACKAGE)
#  define PACKAGE "CBHM"
#endif

#if !defined(APPNAME)
#  define APPNAME "Clipboard History Manager"
#endif

#if !defined(LOCALEDIR)
#  define LOCALEDIR "/usr/share/locale"
#endif

#define EDJ_PATH "/usr/share/edje"
#define APP_EDJ_FILE EDJ_PATH"/cbhmdrawer.edj"
#define GRP_MAIN "cbhmdrawer"

typedef enum _anim_status_t
{
	STATUS_NONE = 0,
	SHOW_ANIM,
	HIDE_ANIM,

	STATUS_MAX
} anim_status_t;

struct appdata
{
	int root_w;
	int root_h;
	Evas *evas;
	Evas_Object *win_main;
	Evas_Object *ly_main; // layout widget based on EDJ 
	/* add more variables here */
	Evas_Object *hig;     // history item gengrid 
	unsigned int hicount; // history item count
	Evas_Object *txtlist;
	anim_status_t anim_status;
	int anim_count;
	Eina_Bool windowshow;
	Eina_Bool pastetextonly;

	Ecore_X_Window active_win;
	int o_degree;
};

void* g_get_main_appdata();

void set_focus_for_app_window(Evas_Object *win, Eina_Bool enable);

#endif /* _cbhm_main_h_ */

