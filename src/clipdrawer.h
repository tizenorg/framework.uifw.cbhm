#ifndef _clipdrawer_h_
#define _clipdrawer_h_

/* view maintains */
int clipdrawer_init(void *data);
int clipdrawer_create_view(void *data);
void clipdrawer_activate_view(void *data);
//void clipdrawer_hide_view(void *data);
void clipdrawer_lower_view(void *data);

const char* clipdrawer_get_plain_string_from_escaped(char *escstr);
int clipdrawer_update_contents(void *data);

#endif // _clipdrawer_h_
