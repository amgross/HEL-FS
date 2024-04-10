
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hel_kernel.h"
#include "mem_driver.h"

// TODO This not protecting against wrapparounds
#define ROUND_UP_DEV(x, y) (((x) + (y) - 1) / y)

static uint8_t *free_map;

#define CHUNK_SIZE_IN_SECTORS(chunk) (!(chunk)->is_file_end ? (chunk)->size : ROUND_UP_DEV((chunk)->size + sizeof(hel_chunk), sector_size))
#define CHUNK_SIZE_IN_BYTES(chunk) ((CHUNK_SIZE_IN_SECTORS(chunk) * sector_size) - sizeof(hel_chunk))
#define CHUNK_DATA_BYTES(chunk) ((chunk)->is_file_end ? (chunk)->size : CHUNK_SIZE_IN_BYTES(chunk))

#define SET_FREE_BIT(id) (free_map[(id) / 8] |= (1 << ((id) % 8)))
#define UNSET_FREE_BIT(id) (free_map[(id) / 8] &= ~(1 << ((id) % 8)))
#define GET_FREE_BIT(id) (free_map[(id) / 8] &  (1 << ((id) % 8)))

#define NUM_OF_SECTORS (mem_size / sector_size)

static uint32_t mem_size;
static uint32_t sector_size;

static hel_ret hel_iterator(hel_chunk *curr_file, hel_file_id id)
{
	hel_file_id next_id;
	hel_ret ret;

	if(NULL == curr_file)
	{
		next_id = 0;
	}
	else
	{
		next_id = id + CHUNK_SIZE_IN_SECTORS(curr_file);
		assert(next_id <= NUM_OF_SECTORS);

		if(next_id >= NUM_OF_SECTORS)
		{
			return hel_mem_err; // This ca be OK
		}
	}

	ret = mem_driver_read(next_id * sector_size, sizeof(*curr_file), (char *)curr_file);
	if(ret != hel_success)
	{
		return ret;
	}
	
	return ret;
}

static hel_ret hel_find_empty_place(hel_file_id id, hel_file_id *out_id)
{
	for(int curr_id = id; curr_id < mem_size / sector_size; curr_id++)
	{
		if(!GET_FREE_BIT(curr_id))
		{
			*out_id = curr_id;
			return hel_success;
		}
	}

	return hel_mem_err; // It may be OK of course
}

static hel_ret hel_sign_area(hel_chunk *_chunk, hel_file_id start_sector_id, bool all_chain)
{
	hel_ret ret;
	hel_chunk chunk = *_chunk;
	uint32_t num_of_sectors = CHUNK_SIZE_IN_SECTORS(&chunk);
	bool fill = (chunk.is_file_begin) ? true: false;

	while(true)
	{
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

		if(!all_chain || chunk.is_file_end)
		{
			break;
		}

		start_sector_id = chunk.next_file_id;

		ret = mem_driver_read(chunk.next_file_id * sector_size, sizeof(chunk), (char *)&chunk);
		if(ret != hel_success)
		{
			// TODO how toheal from this?
			return ret;
		}

		num_of_sectors = CHUNK_SIZE_IN_SECTORS(&chunk);
	}

	return hel_success;
}

static int hel_count_empty_sectors(hel_file_id id)
{
	int ret = 0;
	for(; id < mem_size / sector_size; id++)
	{
		if(GET_FREE_BIT(id))
		{
			return ret;
		}
		else{
			ret++;
		}
	}

	return ret;
}

static hel_ret hel_get_chunks_for_file(int size, hel_file_id **chunks_arr, int *chunks_num)
{
	hel_file_id new_file_id = 0;
	int empty_sectors;
	hel_file_id *chunks_arr_tmp;
	hel_ret ret;

	*chunks_arr = NULL;
	*chunks_num = 0;


	while((ret = hel_find_empty_place(new_file_id, &new_file_id)) == hel_success)
	{			
		empty_sectors = hel_count_empty_sectors(new_file_id);
		chunks_arr_tmp = (hel_file_id *)realloc(*chunks_arr, (*chunks_num + 1) * sizeof(hel_file_id));
		if(chunks_arr_tmp == NULL)
		{
			free(*chunks_arr);
			return hel_out_of_heap_err;
		}

		*chunks_arr = chunks_arr_tmp;
		(*chunks_arr)[*chunks_num] = new_file_id;
		(*chunks_num)++;

		int size_in_empty = (empty_sectors * sector_size) - sizeof(hel_chunk);
		if(size_in_empty >= size)
		{
			break;
		}
		else
		{
			new_file_id += empty_sectors;
			size -= size_in_empty;
		}
	}

	if(ret != hel_success)
	{
		// We will get here also if there is no enough space
		free(*chunks_arr);
		return ret;
	}

	return hel_success;
}

// size is in-out
static hel_ret hel_write_to_chunk(int *size, hel_file_id id, char *buff, bool is_first, hel_file_id next_id)
{
	hel_chunk new_file;
	int empty_sectors;
	hel_ret ret;
	int needed_sectors = ROUND_UP_DEV(*size + sizeof(hel_chunk), sector_size);

	empty_sectors = hel_count_empty_sectors(id);

	// TODO, lots of corner cases here to optimize and for power failure protection
	if(empty_sectors > needed_sectors)
	{ // Need to split
		hel_chunk next_chunk;

		next_chunk.is_file_begin = 0;
		next_chunk.is_file_end = 0;
		next_chunk.size = empty_sectors - needed_sectors;
		ret = mem_driver_write((id + needed_sectors) * sector_size, sizeof(next_chunk), (char *)&next_chunk);
		if(ret != hel_success)
		{
			return ret;
		}

		empty_sectors = needed_sectors;
	}
	
	// TODO there is corner case where there isn't enough sectors because the size is in bytes
	if(empty_sectors < needed_sectors)
	{
		// This is not end of file, size is in sectors.
		new_file.size = empty_sectors;
		new_file.is_file_end = 0;
		*size = (empty_sectors *sector_size) - sizeof(hel_chunk);
	}
	else
	{
		// This is end of file, size is in bytes.
		new_file.size = *size;
		new_file.is_file_end = 1;
	}

	if(is_first)
	{
		new_file.is_file_begin = 1;
	}
	else
	{
		new_file.is_file_begin = 0;
	}

	new_file.next_file_id = next_id;
	
	
	ret = hel_sign_area(&new_file, id, false);
	if(ret != hel_success)
	{
		return ret;
	}

	// 	TODO write both data and metadata together protected against power failure
	ret = mem_driver_write(id * sector_size, sizeof(new_file), (char *)&new_file);
	if(ret != hel_success)
	{
		return ret;
	}

	ret = mem_driver_write((id * sector_size) + sizeof(new_file), *size, buff);
	if(ret != hel_success)
	{
		return ret;
	}

	return hel_success;
} 

hel_ret hel_init()
{
	hel_ret ret;
	hel_chunk check_chunk;
	hel_file_id curr_id = 0;

	ret = mem_driver_init(&mem_size, &sector_size);
	if(ret != hel_success)
	{
		return ret;
	}

	free_map = (uint8_t *)malloc(ROUND_UP_DEV(mem_size / sector_size, 8));
	if(free_map == NULL)
	{
		return hel_out_of_heap_err;
	}

	memset(free_map, 0, ROUND_UP_DEV(mem_size / sector_size, 8));

	while((ret = hel_find_empty_place(curr_id, &curr_id)) != hel_success)
	{ 
		// TODO optimize to not read in case where curr id was empty
		ret = mem_driver_read(curr_id * sector_size, sizeof(check_chunk), (char *)&check_chunk);
		if(ret != hel_success)
		{
			return ret;
		}

		if(check_chunk.is_file_begin)
		{
			// TODO iterate on whole file!
			hel_sign_area(&check_chunk, curr_id, false);
		}
		else
		{
			ret = hel_iterator(&check_chunk, curr_id);
			if(ret != hel_success)
			{
				break;
			}
		}
	}

	if((ret != hel_success) && (ret != hel_mem_err))
	{
		return ret;
	}
	
	return hel_success;
}

hel_ret hel_format()
{
	hel_chunk first_chunk;
	hel_ret ret;

	ret = mem_driver_init(&mem_size, &sector_size);
	if(ret != hel_success)
	{
		return ret;
	}	
	
	assert(sector_size > sizeof(hel_chunk));
	assert(mem_size % sector_size == 0);
	
	// TODO handle cases where the memory range is so big such that need more than one chunk upon format
	first_chunk.size = mem_size / sector_size;
	first_chunk.is_file_begin = 0;
	first_chunk.is_file_end = 0;

	ret = mem_driver_write(0, sizeof(first_chunk), (char *)&first_chunk);
	if(ret != hel_success)
	{
		return ret;
	}

		
	ret = hel_init();
		
	return ret;
}

void print_file(hel_chunk *file, hel_file_id id)
{
	printf("size %d, is file begin %d, is file end %d, id %d\n", file->size, file->is_file_begin, file->is_file_end, id);
}

hel_ret hel_create_and_write(char *in, int size, hel_file_id *out_id)
{
	hel_file_id *new_file_id_arr;
	int chunks_num;
	hel_ret ret;

	if(NULL == out_id)
	{
		return hel_param_err;
	}
	
	ret = hel_get_chunks_for_file(size, &new_file_id_arr, &chunks_num);
	if(ret != hel_success)
	{
		return ret;
	}

	for(int i = 0; i < chunks_num; i++)
	{
		int write_size = size;
		ret = hel_write_to_chunk(&write_size, new_file_id_arr[i], in, i == 0, (i == chunks_num - 1)? 0: new_file_id_arr[i + 1]);
		if(ret != hel_success)
		{
			// TODO, change when will have power failure mechanism
			// TODO ensure in free_mem array all file is free.
			hel_delete(new_file_id_arr[0]);

			free(new_file_id_arr);
			return ret;
		}

		in += write_size;
		size -= write_size;
		assert((i == chunks_num - 1) || (size > 0));
	}

	assert(size == 0);
	
	*out_id = new_file_id_arr[0];

	free(new_file_id_arr);
	
	return hel_success;
}

hel_ret hel_read(hel_file_id id, char *out, int size)
{
	hel_chunk read_file;
	hel_ret ret;

	if(id > NUM_OF_SECTORS)
	{
		return hel_boundaries_err;
	}

	ret = mem_driver_read(id * sector_size, sizeof(read_file), (char *)&read_file);
	if(ret != hel_success)
	{
		return ret;
	}

	if(!read_file.is_file_begin)
	{
		return hel_not_file_err;
	}

	// TODO need much more checks to ensure we are not out of boundaries
	while(size != 0)
	{
		int read_len = (size > CHUNK_DATA_BYTES(&read_file)) ? 
		CHUNK_DATA_BYTES(&read_file): size;
		ret = mem_driver_read((id * sector_size) + sizeof(read_file), read_len, out);
		if(ret != hel_success)
		{
			return ret;
		}

		out += read_len;
		size -= read_len;
		if(read_file.is_file_end)
		{
			if(size != 0)
			{
				return hel_boundaries_err;
			}
		}
		else
		{
			id = read_file.next_file_id;
			ret = mem_driver_read(id * sector_size, sizeof(read_file), (char *)&read_file);
			if(ret != hel_success)
			{
				return ret;
			}
		}
	}

	return 0;
}

hel_ret hel_delete(hel_file_id id)
{
	hel_chunk del_file;
	hel_ret ret;

	if(id > NUM_OF_SECTORS)
	{
		return hel_boundaries_err;
	}

	ret = mem_driver_read(id * sector_size, sizeof(del_file), (char *)&del_file);
	if(ret != hel_success)
	{
		return ret;
	}


	if(!del_file.is_file_begin)
	{
		return hel_not_file_err;
	}

	del_file.is_file_begin = 0;

	ret = hel_sign_area(&del_file, id, true);
	if(ret != hel_success)
	{
		return ret;
	}

	ret = mem_driver_write(id * sector_size, sizeof(del_file), (char *)&del_file);
	if(ret != hel_success)
	{
		return ret;
	}

	return hel_success;
}
