
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "hel_kernel.h"

static uint8_t mem_buff[MEM_SIZE];

hel_ret init_fs()
{
	hel_chunk *start_pointer = (hel_chunk *)mem_buff;

	assert(SECTOR_SIZE > sizeof(hel_chunk));

	start_pointer->size = MEM_SIZE - sizeof(hel_chunk);
	start_pointer->is_file_begin = 0;
	
	return hel_success;
}

void print_file(hel_chunk *file)
{
	printf("size %d, is file begin %d, is file end %d, diff from begin %ld\n", file->size, file->is_file_begin, file->is_file_end, (uint8_t *)file - mem_buff);
}

static hel_chunk *hel_iterator(hel_chunk *curr_file)
{
	hel_chunk *next_chunk;

	if(NULL == curr_file)
	{
		next_chunk = (hel_chunk *)mem_buff;

		return next_chunk;
	}

	next_chunk = (hel_chunk *)(curr_file->data + curr_file->size);
	if((uint8_t *)next_chunk < mem_buff + MEM_SIZE)
	{
		return next_chunk;
	}

	assert((uint8_t *)next_chunk == mem_buff + MEM_SIZE);
	
	return NULL;
}

static hel_chunk *hel_find_empty_place(int size)
{
	hel_chunk *curr_file = NULL;
	while((curr_file = hel_iterator(curr_file)) != NULL)
	{
		// TODO add option to concatinate 
		if((curr_file->is_file_begin == 0) && (curr_file->size >= size))
		{
			return curr_file;
		}
	}

	return NULL;
}

hel_ret create_and_write(char *in, int size, hel_file_id *out_id)
{
	hel_chunk *new_file;

	if(NULL == out_id)
	{
		return hel_param_err;
	}
	
	new_file = hel_find_empty_place(size);
	if(new_file == NULL)
	{
		return hel_mem_err;
	}

	if(new_file->size > size)
	{
		hel_chunk *next_chunk;

		next_chunk = (hel_chunk *)(new_file->data + size);
		next_chunk->is_file_begin = 0;
		next_chunk->size = new_file->size - sizeof(hel_chunk) - size;
	}

	new_file->size = size;
	new_file->is_file_begin = 1;
	new_file->is_file_end = 1;

	memcpy(new_file->data, in, size);

	*out_id = (uint8_t *)new_file - mem_buff;

	return 0;
}

hel_ret read_file(hel_file_id id, char *out, int size)
{
	hel_chunk *read_file = (hel_chunk *)(mem_buff + id);

	if(!read_file->is_file_begin)
	{
		return hel_not_file_err;
	}

	// TODO need much more checks to ensure we are not out of boundaries
	if(read_file->size < size)
	{
		return hel_boundaries_err;
	}

	memcpy(out, read_file->data, size);
	return 0;
}

hel_ret hel_delete_file(hel_file_id id)
{
	hel_chunk *del_file = (hel_chunk *)(mem_buff + id);

	if(!del_file->is_file_begin)
	{
		return hel_not_file_err;
	}

	del_file->is_file_begin = 0;

	return hel_success;
}
