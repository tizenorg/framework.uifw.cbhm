#ifndef _storage_h_
#define _storage_h_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int init_storage(void *data);
int sync_storage(void *data);
unsigned int get_storage_serial_code(void *data);
int adding_item_to_storage(void *data, int pos, char *idata);
int get_item_counts(void *data);
int close_storage(void *data);

int check_regular_file(char *path);

#endif // _storage_h_
