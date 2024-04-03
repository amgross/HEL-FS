
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#include "hel_kernel.h"

static uint8_t mem_buff[MEM_SIZE];

// TODO This not protecting against wrapparounds
#define ROUND_UP_DEV(x, y) (((x) + (y) - 1) / y)

static uint8_t free_map[ROUND_UP_DEV(MEM_SIZE / SECTOR_SIZE, 8)];

#define CHUNK_SIZE_IN_SECTORS(chunk) (!chunk->is_file_end ? chunk->size : ROUND_UP_DEV(chunk->size + sizeof(hel_chunk), SECTOR_SIZE))
#define CHUNK_SIZE_IN_BYTES(chunk) ((CHUNK_SIZE_IN_SECTORS(chunk) * SECTOR_SIZE) - sizeof(hel_chunk))
#define CHUNK_DATA_BYTES(chunk) (chunk->is_file_end ? chunk->size : CHUNK_SIZE_IN_BYTES(chunk))

#define CHUNK_TO_ID(chunk) (((uint8_t *)(chunk) - mem_buff) / SECTOR_SIZE)
#define ID_TO_CHUNK(id) ((hel_chunk *)(mem_buff + ((id) * SECTOR_SIZE)))

#define SET_FREE_BIT(id) (free_map[(id) / 8] |= (1 << ((id) % 8)))
#define UNSET_FREE_BIT(id) (free_map[(id) / 8] &= ~(1 << ((id) % 8)))
#define GET_FREE_BIT(id) (free_map[(id) / 8] &  (1 << ((id) % 8)))


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

static hel_chunk *hel_find_empty_place(hel_file_id id)
{
	for(int curr_id = id; curr_id < MEM_SIZE / SECTOR_SIZE; curr_id++)
	{
		if(!GET_FREE_BIT(curr_id))
		{
			return (hel_chunk *)&mem_buff[curr_id * SECTOR_SIZE];
		}
	}

	return NULL;
}

static void hel_sign_area(hel_chunk *chunk, bool fill)
{
	uint32_t start_sector_id = CHUNK_TO_ID(chunk);
	uint32_t num_of_sectors = CHUNK_SIZE_IN_SECTORS(chunk);

	for(hel_file_id id = start_sector_id; id < start_sector_id + num_of_sectors; id++)
	{
		if(fill)
		{
			SET_FREE_BIT(id);
		}
		else
		{
			UNSET_FREE_BIT(id);
		}
	}
}

hel_ret hel_init()
{
	hel_chunk *check_chunk;
	hel_file_id curr_id = 0;

	memset(free_map, 0, sizeof(free_map));

	while((check_chunk = hel_find_empty_place(curr_id)) != NULL)
	{
		if(check_chunk->is_file_begin)
		{
			hel_sign_area(check_chunk, true);
		}
		else
		{
			check_chunk = hel_iterator(check_chunk);
			if(NULL == check_chunk)
			{
				break;
			}
			curr_id = CHUNK_TO_ID(check_chunk);
		}
	}

	return hel_success;
}

hel_ret hel_format()
{
	hel_chunk *start_pointer = (hel_chunk *)mem_buff;
	hel_ret ret;
	
	assert(SECTOR_SIZE > sizeof(hel_chunk));
	assert(MEM_SIZE % SECTOR_SIZE == 0);
	
	start_pointer->size = MEM_SIZE / SECTOR_SIZE;
	start_pointer->is_file_begin = 0;
	start_pointer->is_file_end = 0;
		
	ret = hel_init();
		
	return ret;
}

void print_file(hel_chunk *file)
{
	printf("size %d, is file begin %d, is file end %d, id %ld\n", file->size, file->is_file_begin, file->is_file_end, CHUNK_TO_ID(file));
}

hel_ret hel_create_and_write(char *in, int size, hel_file_id *out_id)
{
	hel_chunk *new_file;
	hel_file_id curr_id = 0;

	if(NULL == out_id)
	{
		return hel_param_err;
	}
	
	while((new_file = hel_find_empty_place(curr_id)) != NULL)
	{	
		// TODO concatinate chunks if it helpful
		if(CHUNK_SIZE_IN_BYTES(new_file) >= size)
		{
			break;
		}
		else
		{	

			new_file = hel_iterator(new_file);	

			if(new_file == NULL)
			{	

				break;
			}	

			curr_id = CHUNK_TO_ID(new_file);
		}
	}
	
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
	
	hel_sign_area(new_file, true);

	memcpy(new_file->data, in, size);
	
	*out_id = CHUNK_TO_ID(new_file);
	
	return 0;
}

hel_ret hel_read(hel_file_id id, char *out, int size)
{
	hel_chunk *read_file = ID_TO_CHUNK(id);

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
	hel_chunk *del_file = ID_TO_CHUNK(id);

	if(!del_file->is_file_begin)
	{
		return hel_not_file_err;
	}

	del_file->size = CHUNK_SIZE_IN_SECTORS(del_file);
	del_file->is_file_begin = 0;
	del_file->is_file_end = 0;

	hel_sign_area(del_file, false);

	return hel_success;
}
