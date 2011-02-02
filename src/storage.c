#include "common.h"
#include "storage.h"

/*
   file structure 

   /---------------------------------------------------------------------------
    |header|current_position|total_count|item_header|item_body(512kib)|item...|
    --------------------------------------------------------------------------/

*/

#define STORAGE_FILEPATH "/opt/var/.savecbh"
#define STORAGE_MAX_ITEMS HISTORY_QUEUE_MAX_ITEMS
#define HEADER_ITEM_SIZE (sizeof(int)) 
#define BODY_ITEM_SIZE HISTORY_QUEUE_ITEM_SIZE
#define STORAGE_HEADER_SIZE (STORAGE_MAX_ITEMS * HEADER_ITEM_SIZE)
#define STORAGE_BODY_SIZE (STORAGE_MAX_ITEMS * BODY_ITEM_SIZE) 
#define TOTAL_STORAGE_SIZE (STORAGE_HEADER_SIZE+STORAGE_BODY_SIZE)

#define GET_ITEM_ADDR_BY_POSITION(map, pos) (map+STORAGE_HEADER_SIZE+pos*BODY_ITEM_SIZE)

static int g_storage_fd = 0;
static unsigned int g_storage_serial_number = 0;
static char *g_map = NULL;

int init_storage()
{
	int i;
	int result = 0;

	if (g_storage_fd != 0)
		return 1;

	g_storage_fd = open(STORAGE_FILEPATH, O_RDWR | O_CREAT, (mode_t)0600);
	if (g_storage_fd == -1)
	{
		g_storage_fd = 0;
		close_storage();
		DTRACE("Error : failed openning file for writing\n");
		return -1;
	}

    result = lseek(g_storage_fd, TOTAL_STORAGE_SIZE-1, SEEK_SET);
    if (result == -1) 
	{
		close_storage();
		DTRACE("Error : failed moving file position to file's end\n");
		return -1;
    }
    
    result = write(g_storage_fd, "", 1);
    if (result != 1) 
	{
		close_storage();
		DTRACE("Error : failed writing to file's end\n");
		return -1;
    }

	g_map = mmap(0, TOTAL_STORAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_storage_fd, 0);
	if (g_map == MAP_FAILED)
	{
		close_storage();
		DTRACE("Error : failed mmapping the file\n");
		return -1;
	}

	// FIXME : do not unconditionaly initialize, maybe old data can be
	// at here
	int *header = g_map;
	for (i = 0; i < STORAGE_MAX_ITEMS; i++)
	{
		char d = 0;
		header[i] = 0;
		memcpy(GET_ITEM_ADDR_BY_POSITION(g_map, i), &d, 1);
	}
	DTRACE("Success : storage init is done\n");

	g_storage_serial_number = 0;

	return 0;
}

int sync_storage()
{
	if (g_map == NULL)
	{
		DTRACE("g_map is null\n");
		return -1;
	}
	msync(g_map, TOTAL_STORAGE_SIZE, MS_ASYNC);

	return 0;
}

int get_total_storage_size()
{
	return TOTAL_STORAGE_SIZE;
}

unsigned int get_storage_serial_code()
{
	return g_storage_serial_number;
}

int adding_item_to_storage(int pos, char *data)
{
	int *header = get_storage_start_addr();

	if (g_map == NULL)
	{
		DTRACE("g_map is null");
		return -1;
	}
	// saving relative addr at header
	header[pos] = STORAGE_HEADER_SIZE+pos*BODY_ITEM_SIZE; 
	memcpy(GET_ITEM_ADDR_BY_POSITION(g_map, pos), data, BODY_ITEM_SIZE);
	g_storage_serial_number++;
	return 0;
}

char *get_storage_start_addr()
{
	return g_map;
}

int get_item_counts()
{
	int i, count;
	int *header = get_storage_start_addr();

	count = 0;
	for (i = 0; i < STORAGE_MAX_ITEMS; i++)
	{
		if (header[i] != 0)
			count++;
	}
	return count;
}

char *get_item_contents_by_pos(int pos)
{
	if (g_map == NULL)
	{
		DTRACE("g_map is null");
		return NULL;
	}
	return GET_ITEM_ADDR_BY_POSITION(g_map, pos);
}

int close_storage()
{
	if (g_map)
		munmap(g_map, TOTAL_STORAGE_SIZE);
	g_map = NULL;

	if (g_storage_fd)
		close(g_storage_fd);
	g_storage_fd = 0;
	g_storage_serial_number = 0;

	return 0;
}

int check_regular_file(char *path)
{
	struct stat fattr;
	if (stat(path, &fattr))
	{
		DTRACE("Cannot get file at path = %s\n", path);
		return FALSE;
	}
	
	return S_ISREG(fattr.st_mode);
}

