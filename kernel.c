
#include <stdint.h>
#include <string.h>
#include "kernel.h"

#define EMPTY_SIZE 0xffffffff

static uint8_t mem_buff[MEM_SIZE];

hel_ret init_fs()
{
	memset(mem_buff, 0xff, MEM_SIZE);
	
	return hel_success;
}

hel_file *find_empty_place(int size)
{
	for(hel_file *curr_file = (hel_file *)mem_buff;; curr_file = (hel_file *)((uint8_t *)curr_file + sizeof(hel_file) + curr_file->size))
	{
		// TODO, ensure no wrap around
		if((uint8_t *)curr_file + sizeof(hel_file) + size > mem_buff + MEM_SIZE)
		{
			return NULL;
		}

		if(curr_file->size == EMPTY_SIZE)
		{
			return curr_file;
		}
	}
}

hel_ret create_and_write(char *in, int size, hel_file_id *out_id)
{
	hel_file *new_file;

	if(NULL == out_id)
	{
		return hel_param_err;
	}
	
	new_file = find_empty_place(size);
	if(new_file == NULL)
	{
		return hel_mem_err;
	}

	new_file->size = size;

	memcpy(new_file->data, in, size);

	*out_id = (uint8_t *)new_file - mem_buff;

	return 0;
}

hel_ret read_file(hel_file_id id, char *out, int size)
{
	hel_file *read_file = (hel_file *)(mem_buff + id);

	// TODO need much more checks to ensure we are not out of boundaries
	if(read_file->size < size)
	{
		return hel_boundaries_err;
	}

	memcpy(out, read_file->data, size);
	return 0;
}
