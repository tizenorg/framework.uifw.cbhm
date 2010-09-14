#ifndef _common_h_
#define _common_h_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG 

#ifdef DEBUG
#define DTRACE(fmt, args...) \
{do { fprintf(stderr, "[CBHM][%s:%04d] " fmt, __func__,__LINE__, ##args); } while (0); }
#else
#define DTRACE(fmt, args...) 
#endif

struct appdata;

#define HISTORY_QUEUE_NUMBER 5

#endif // _common_h_
