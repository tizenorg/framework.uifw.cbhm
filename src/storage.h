#ifndef _storage_h_
#define _storage_h_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int init_storage();
int sync_storage();
unsigned int get_storage_serial_code();
int adding_item_to_storage(int pos, char *data);
char *get_storage_start_addr();
int get_item_counts();
char *get_item_contents_by_pos(int pos);
int close_storage();

#endif // _storage_h_
