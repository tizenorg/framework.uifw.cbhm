#ifndef _scrcapture_h_
#define _scrcapture_h_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/XShm.h>
#include <X11/Xatom.h>

// XV extension API - start 
const char* createScreenShot(int width, int height);
void releaseScreenShot(const char *ss);
// XV extension API - end

char *scrcapture_screen_capture_by_x11(Window xid, int *size);
char *scrcapture_screen_capture_by_xv_ext(int width, int height);

#endif // _scrcapture_h_
