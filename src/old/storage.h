/*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the License);
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  * http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an AS IS BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */




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
