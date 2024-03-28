
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"

static uint8_t mem_buff[MEM_SIZE];

typedef struct my_code
{
	int size;
	char data[];
}file;


helfs_ret init_fs()
{
	memset(mem_buff, 0, MEM_SIZE);
	
	return helfs_success;
}

file *find_empty_place(int size)
{
	for(file *curr_file = (file *)mem_buff;; curr_file = (file *)((uint8_t *)curr_file + sizeof(file) + curr_file->size))
	{
		// TODO, ensure no wrap around
		if((uint8_t *)curr_file + sizeof(file) + size > mem_buff + MEM_SIZE)
		{
			return NULL;
		}

		if(curr_file->size == 0)
		{
			return curr_file;
		}
	}
}

helfs_ret create_and_write(char *in, int size, file_id *out_id)
{
	file *new_file;

	if(NULL == out_id)
	{
		return halfs_param_err;
	}
	
	new_file = find_empty_place(size);
	if(new_file == NULL)
	{
		return helfs_mem_err;
	}

	new_file->size = size;

	memcpy(new_file->data, in, size);

	*out_id = (uint8_t *)new_file - mem_buff;

	return 0;
}

helfs_ret read_file(file_id id, char *out, int size)
{
	file *read_file = (file *)(mem_buff + id);

	// TODO need much more checks to ensure we are not out of boundaries
	if(read_file->size < size)
	{
		return helfs_boundaries_err;
	}

	memcpy(out, read_file->data, size);
	return 0;
}
