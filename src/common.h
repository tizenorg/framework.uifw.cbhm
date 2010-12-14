#ifndef _common_h_
#define _common_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <Elementary.h>
#include <Ecore_X.h>
#include <utilX.h>
//#include <appcore-efl.h>

#ifndef _EDJ
#define _EDJ(ly) elm_layout_edje_get(ly)
#endif

#define DEBUG 

#ifdef DEBUG
#define DTRACE(fmt, args...) \
{do { fprintf(stderr, "[CBHM][%s:%04d] " fmt, __func__,__LINE__, ##args); } while (0); }
#define DTIME(fmt, args...)														\
{do { struct timeval tv1; gettimeofday(&tv1, NULL); double t1=tv1.tv_sec+(tv1.tv_usec/1000000.0); fprintf(stderr, "[CBHM][time=%lf:%s:%04d] " fmt, t1, __func__, __LINE__, ##args); } while (0); }
#else
#define DTRACE(fmt, args...) 
#define DTIME(fmt, args...) 
#endif

struct appdata;

#define HISTORY_QUEUE_ITEM_SIZE (5 * 1024) // 4Kilo 
#define HISTORY_QUEUE_MAX_ITEMS 12

#endif // _common_h_
