
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "hel_kernel.h"

static uint8_t mem_buff[MEM_SIZE];

hel_ret init_fs()
{
	hel_file *start_pointer = (hel_file *)mem_buff;

	start_pointer->size = MEM_SIZE - sizeof(hel_file);
	start_pointer->is_file_begin = 0;
	
	return hel_success;
}

void print_file(hel_file *file)
{
	printf("size %d, is file begin %d, is file end %d, diff from begin %ld\n", file->size, file->is_file_begin, file->is_file_end, (uint8_t *)file - mem_buff);
}

static hel_file *hel_iterator(hel_file *curr_file)
{
	hel_file *next_file;

	if(NULL == curr_file)
	{
		next_file = (hel_file *)mem_buff;

		return next_file;
	}

	next_file = (hel_file *)(curr_file->data + curr_file->size);
	if((uint8_t *)next_file < mem_buff + MEM_SIZE)
	{
		return next_file;
	}

	// TODO assert?
	assert((uint8_t *)next_file == mem_buff + MEM_SIZE);
	
	return NULL;
}

static hel_file *hel_find_empty_place(int size)
{
	hel_file *curr_file = NULL;
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
	hel_file *new_file;

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
		hel_file *next_file;

		next_file = (hel_file *)(new_file->data + size);
		next_file->is_file_begin = 0;
		next_file->size = new_file->size - sizeof(hel_file);
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
	hel_file *read_file = (hel_file *)(mem_buff + id);

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
	hel_file *del_file = (hel_file *)(mem_buff + id);

	if(!del_file->is_file_begin)
	{
		return hel_not_file_err;
	}

	del_file->is_file_begin = 0;

	return hel_success;
}
