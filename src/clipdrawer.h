#ifndef _clipdrawer_h_
#define _clipdrawer_h_

static int clipdrawer_init();
int clipdrawer_update_contents(void *data);
int clipdrawer_create_view();
void clipdrawer_activate_view();
void clipdrawer_hide_view(void *data);

#endif // _clipdrawer_h_
