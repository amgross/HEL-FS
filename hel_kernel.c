
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "hel_kernel.h"

static uint8_t mem_buff[MEM_SIZE];

// TODO This not protecting against wrapparounds
#define ROUND_UP_DEV(x, y) (((x) + (y) - 1) / y)

#define CHUNK_SIZE_IN_SECTORS(chunk) (!chunk->is_file_end ? chunk->size : ROUND_UP_DEV(chunk->size + sizeof(hel_chunk), SECTOR_SIZE))
#define CHUNK_SIZE_IN_BYTES(chunk) ((CHUNK_SIZE_IN_SECTORS(chunk) * SECTOR_SIZE) - sizeof(hel_chunk))
#define CHUNK_DATA_BYTES(chunk) (chunk->is_file_end ? chunk->size : CHUNK_SIZE_IN_BYTES(chunk))

hel_ret hel_init()
{
	return hel_success;
}

hel_ret hel_format()
{
	hel_chunk *start_pointer = (hel_chunk *)mem_buff;

	assert(SECTOR_SIZE > sizeof(hel_chunk));
	assert(MEM_SIZE % SECTOR_SIZE == 0);

	start_pointer->size = MEM_SIZE / SECTOR_SIZE;
	start_pointer->is_file_begin = 0;
	start_pointer->is_file_end = 0;
	
	return hel_init();
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

	next_chunk = (hel_chunk *)(curr_file->data + CHUNK_SIZE_IN_BYTES(curr_file));
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
		if((curr_file->is_file_begin == 0) && (CHUNK_SIZE_IN_BYTES(curr_file) >= size))
		{
			return curr_file;
		}
	}

	return NULL;
}

hel_ret hel_create_and_write(char *in, int size, hel_file_id *out_id)
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

	if(CHUNK_SIZE_IN_SECTORS(new_file) > ROUND_UP_DEV(size + sizeof(hel_chunk), SECTOR_SIZE))
	{ // Need to split
		int new_chunk_sector_size = ROUND_UP_DEV(size + sizeof(hel_chunk), SECTOR_SIZE);
		hel_chunk *next_chunk;

		next_chunk = (hel_chunk *)(new_file->data + ((new_chunk_sector_size * SECTOR_SIZE) - sizeof(hel_chunk)));
		next_chunk->is_file_begin = 0;
		next_chunk->is_file_end = 0;
		next_chunk->size = CHUNK_SIZE_IN_SECTORS(new_file) - new_chunk_sector_size;
	}

	new_file->size = size;
	new_file->is_file_begin = 1;
	new_file->is_file_end = 1;

	memcpy(new_file->data, in, size);

	*out_id = (uint8_t *)new_file - mem_buff;

	return 0;
}

hel_ret hel_read(hel_file_id id, char *out, int size)
{
	hel_chunk *read_file = (hel_chunk *)(mem_buff + id);

	if(!read_file->is_file_begin)
	{
		return hel_not_file_err;
	}

	// TODO need much more checks to ensure we are not out of boundaries
	if(CHUNK_DATA_BYTES(read_file) < size)
	{
		return hel_boundaries_err;
	}

	memcpy(out, read_file->data, size);
	return 0;
}

hel_ret hel_delete(hel_file_id id)
{
	hel_chunk *del_file = (hel_chunk *)(mem_buff + id);

	if(!del_file->is_file_begin)
	{
		return hel_not_file_err;
	}

	del_file->size = CHUNK_SIZE_IN_SECTORS(del_file);
	del_file->is_file_begin = 0;
	del_file->is_file_end = 0;

	return hel_success;
}
