
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

#define CHUNK_SIZE_IN_SECTORS(chunk) (!(chunk)->is_file_end ? (chunk)->type.not_end.sectors_size : ROUND_UP_DEV((chunk)->type.end.bytes_size + sizeof(hel_chunk), sector_size))
#define CHUNK_SIZE_IN_BYTES(chunk) ((CHUNK_SIZE_IN_SECTORS(chunk) * sector_size) - sizeof(hel_chunk))
#define CHUNK_DATA_BYTES(chunk) ((chunk)->is_file_end ? (chunk)->type.end.bytes_size : CHUNK_SIZE_IN_BYTES(chunk))

#define SET_FREE_BIT(id) (free_map[(id) / 8] |= (1 << ((id) % 8)))
#define UNSET_FREE_BIT(id) (free_map[(id) / 8] &= ~(1 << ((id) % 8)))
#define GET_FREE_BIT(id) (free_map[(id) / 8] &  (1 << ((id) % 8)))

#define NUM_OF_SECTORS (mem_size / sector_size)

#define PROTECT_POWER_LOSS

#define HEL_MIN(x, y) ((x > y) ? y: x)

/*
 * @brief Internal macro for reading chunk metadata from memory.
 * 
 * @param id [IN] The id of the chunk to read.
 * @param p_chunk [OUT] The chunk from memory.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
*/
#define READ_CHUNK_METADATA(id, p_chunk) mem_driver_read((id) * sector_size, sizeof(hel_chunk), (p_chunk))

static uint32_t mem_size;
static uint32_t sector_size;

typedef struct
{
	hel_file_id id;
	int size;
}chunk_data;

/*
 * @brief internal function to iterate over chunks.
 * 
 * @param [INOUT] curr_chunk - pointer to chunk, upon success points to next chunk.
 * @param [INOUT] id - pointer to the id of curr_chunk, upon success points to the next chunk id.
 * 
 * @return hel_success upon success, hel_mem_err in case curr_chunk is last chunk, hel_XXXX_err otherwise.
 */
static hel_ret hel_iterator(hel_chunk *curr_chunk, hel_file_id *id)
{
	hel_file_id next_id;
	hel_ret ret;

	next_id = *id + CHUNK_SIZE_IN_SECTORS(curr_chunk);
	assert(next_id <= NUM_OF_SECTORS);
	assert(next_id > *id);

	if(next_id >= NUM_OF_SECTORS)
	{
		return hel_mem_err;
	}

	ret = READ_CHUNK_METADATA(next_id, curr_chunk);
	if(ret != hel_success)
	{
		return ret;
	}

	*id = next_id;
	
	return hel_success;
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

	return hel_mem_err;
}

static hel_ret hel_sign_area(hel_chunk *_chunk, hel_file_id start_sector_id, bool all_chain, bool fill)
{
	hel_ret ret;
	hel_chunk chunk = *_chunk;
	uint32_t num_of_sectors = CHUNK_SIZE_IN_SECTORS(&chunk);

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

		start_sector_id = chunk.type.not_end.next_file_id;

		ret = READ_CHUNK_METADATA(chunk.type.not_end.next_file_id, &chunk);
		if(ret != hel_success)
		{
			// TODO how to heal from this?
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

static hel_ret mem_write_one_helper(uint32_t v_addr, int size, void *in)
{
	return mem_driver_write(v_addr, &size, &in, 1);
}

static hel_ret hel_get_chunks_for_file(int size, chunk_data **chunks_arr, int *chunks_num)
{
	hel_file_id new_file_id = 0;
	int empty_sectors;
	chunk_data *chunks_arr_tmp;
	hel_ret ret;

	*chunks_arr = NULL;
	*chunks_num = 0;


	while((ret = hel_find_empty_place(new_file_id, &new_file_id)) == hel_success)
	{			
		empty_sectors = hel_count_empty_sectors(new_file_id);
		chunks_arr_tmp = (chunk_data *)realloc(*chunks_arr, (*chunks_num + 1) * sizeof(chunk_data));
		if(chunks_arr_tmp == NULL)
		{
			free(*chunks_arr);
			return hel_out_of_heap_err;
		}

		*chunks_arr = chunks_arr_tmp;
		(*chunks_arr)[*chunks_num].id = new_file_id;

		int size_in_empty = (empty_sectors * sector_size) - sizeof(hel_chunk);
		if(size_in_empty >= size)
		{
			(*chunks_arr)[*chunks_num].size = size;
			(*chunks_num)++;
			break;
		}
		else
		{
			new_file_id += empty_sectors;
			size -= size_in_empty;

			(*chunks_arr)[*chunks_num].size = size_in_empty;
			(*chunks_num)++;
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

static hel_ret hel_organize_chunks_arr(chunk_data *chunks_arr, int chunks_num)
{
	hel_ret ret;

	for(int i = 0; i < chunks_num; i++)
	{
		hel_chunk first_chunk, curr_chunk;
		/*
		 * options:
		 * 1. all perfect.
		 * 2. need to update first for defragment, no need to update after as beginning of chunk
		 * 3. need to update first and create second that not exist.
		 * 
		 * what needed to check:
		 * if after not exist, else if first is fragmented
		 */
		bool need_to_update_first = false, need_to_update_first_and_end = false;
		int empty_sectors = hel_count_empty_sectors(chunks_arr[i].id);
		int needed_sectors = ROUND_UP_DEV(chunks_arr[i].size + sizeof(hel_chunk), sector_size);
		ret = mem_driver_read(chunks_arr[i].id * sector_size, sizeof(first_chunk), &first_chunk);
		if(ret != hel_success)
		{
			return ret;
		}

		if(CHUNK_SIZE_IN_SECTORS(&first_chunk) != needed_sectors)
		{
			curr_chunk = first_chunk;
			need_to_update_first = true;
			hel_file_id curr_id= chunks_arr[i].id;
			while(true)
			{
				curr_id += CHUNK_SIZE_IN_SECTORS(&curr_chunk);
				if(curr_id > chunks_arr[i].id + needed_sectors)
				{
					need_to_update_first_and_end = true;
					break;
				}
				
				if(curr_id == chunks_arr[i].id + needed_sectors)
				{
					break;
				}

				ret = READ_CHUNK_METADATA(curr_id, &curr_chunk);
				if(ret != hel_success)
				{
					return ret;
				}
			}
		}

		if(need_to_update_first_and_end)
		{
			// write_end
			hel_chunk end_chunk = {.is_file_begin = 0, .is_file_end = 0, .type.not_end.next_file_id = 0, .type.not_end.sectors_size = empty_sectors - needed_sectors};
			mem_write_one_helper((chunks_arr[i].id + needed_sectors) * sector_size, sizeof(hel_chunk), &end_chunk);
		}

#ifdef PROTECT_POWER_LOSS
		if(need_to_update_first || need_to_update_first_and_end)
		{
			// write_first
			hel_chunk first_chunk = {.is_file_begin = 0, .is_file_end = 0, .type.not_end.next_file_id = 0, .type.not_end.sectors_size = needed_sectors};
			mem_write_one_helper(chunks_arr[i].id * sector_size, sizeof(hel_chunk), &first_chunk);
		}
#endif
	}

	return hel_success;
}

// size is in-out
static hel_ret hel_write_to_chunk(int size, hel_file_id id, void *buff, bool is_first, bool is_end, hel_file_id next_id)
{
	hel_chunk new_file;
	void *p_chunk_for_mem;
	void *pp_chunk_for_mem[2];
	int p_size_for_mem[2];
	hel_ret ret;
	int needed_sectors = ROUND_UP_DEV(size + sizeof(hel_chunk), sector_size);

	if(!is_end)
	{
		// This is not end of file, size is in sectors.
		new_file.type.not_end.sectors_size = needed_sectors;
		new_file.is_file_end = 0;
		new_file.type.not_end.next_file_id = next_id;
		size = (needed_sectors * sector_size) - sizeof(hel_chunk);
	}
	else
	{
		// This is end of file, size is in bytes.
		new_file.type.end.bytes_size = size;
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
	
	ret = hel_sign_area(&new_file, id, false, true);
	if(ret != hel_success)
	{
		return ret;
	}

	p_chunk_for_mem = &new_file;
	pp_chunk_for_mem[0] = p_chunk_for_mem;
	p_size_for_mem[0] = sizeof(new_file);

	pp_chunk_for_mem[1] = buff;
	p_size_for_mem[1] = size;
	ret = mem_driver_write(id * sector_size, p_size_for_mem, pp_chunk_for_mem, 2);
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

	while((ret = hel_find_empty_place(curr_id, &curr_id)) == hel_success)
	{ 
		ret = READ_CHUNK_METADATA(curr_id, &check_chunk);
		if(ret != hel_success)
		{
			return ret;
		}

		if(check_chunk.is_file_begin)
		{
			hel_sign_area(&check_chunk, curr_id, true, true);
		}
		else
		{
			ret = hel_iterator(&check_chunk, &curr_id);
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

hel_ret hel_close()
{
	hel_ret ret;

	free(free_map);
	free_map = NULL;

	ret = mem_driver_close();
	if(ret != hel_success)
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
	first_chunk.type.not_end.sectors_size = mem_size / sector_size;
	first_chunk.is_file_begin = 0;
	first_chunk.is_file_end = 0;

	ret = mem_write_one_helper(0, sizeof(first_chunk), &first_chunk);
	if(ret != hel_success)
	{
		return ret;
	}

		
	ret = hel_init();
		
	return ret;
}

hel_ret hel_create_and_write(void *_in, int size, hel_file_id *out_id)
{
	uint8_t *in = _in;
	chunk_data *new_chunks_arr;
	int chunks_num;
	hel_ret ret;

	if(NULL == out_id)
	{
		return hel_param_err;
	}
	
	ret = hel_get_chunks_for_file(size, &new_chunks_arr, &chunks_num);
	if(ret != hel_success)
	{
		return ret;
	}

	ret = hel_organize_chunks_arr(new_chunks_arr, chunks_num);
	if(ret != hel_success)
	{
		free(new_chunks_arr);
		return ret;
	}

	// We are writing from end to start (due to power down protection)
	in += size;

	for(int i = chunks_num - 1; i != -1; i--)
	{
		int write_size = new_chunks_arr[i].size;
		in -= write_size;

		ret = hel_write_to_chunk(write_size, new_chunks_arr[i].id, in, i == 0, i == chunks_num - 1, (i == chunks_num - 1)? 0: new_chunks_arr[i + 1].id);
		if(ret != hel_success)
		{
			/// No need to delete something in case of failure, if not all chunks written so nothing really done.

			free(new_chunks_arr);
			return ret;
		}

		size -= write_size;
		assert((i == 0) || (size > 0));
	}

	assert(size == 0);
	
	*out_id = new_chunks_arr[0].id;

	free(new_chunks_arr);
	
	return hel_success;
}

hel_ret hel_read(hel_file_id id, void *_out, int begin, int size)
{
	uint8_t *out = _out;
	hel_chunk read_file;
	hel_ret ret;

	if(id > NUM_OF_SECTORS)
	{
		return hel_boundaries_err;
	}

	ret = READ_CHUNK_METADATA(id, &read_file);
	if(ret != hel_success)
	{
		return ret;
	}

	if(!read_file.is_file_begin)
	{
		return hel_not_file_err;
	}

	while(size != 0)
	{
		int begin_offset = HEL_MIN(CHUNK_DATA_BYTES(&read_file), begin);
		begin -= begin_offset;
		int read_len = (size > CHUNK_DATA_BYTES(&read_file) - begin_offset) ? CHUNK_DATA_BYTES(&read_file) - begin_offset: size;

		if(read_len != 0)
		{
			ret = mem_driver_read((id * sector_size) + sizeof(read_file) + begin_offset, read_len, out);
			if(ret != hel_success)
			{
				return ret;
			}
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
			id = read_file.type.not_end.next_file_id;
			ret = READ_CHUNK_METADATA(id, &read_file);
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

	ret = READ_CHUNK_METADATA(id, &del_file);
	if(ret != hel_success)
	{
		return ret;
	}

	if(!del_file.is_file_begin)
	{
		return hel_not_file_err;
	}

	del_file.is_file_begin = 0;

	ret = hel_sign_area(&del_file, id, true, false);
	if(ret != hel_success)
	{
		return ret;
	}

	ret = mem_write_one_helper(id * sector_size, sizeof(del_file), &del_file);
	if(ret != hel_success)
	{
		return ret;
	}

	return hel_success;
}

hel_ret hel_get_first_file(hel_file_id *id, hel_chunk  *file)
{
	hel_ret ret;
	hel_chunk curr_file;

	ret = READ_CHUNK_METADATA(0, &curr_file);
	if(ret != hel_success)
	{
		return ret;
	}
	
	*id = 0;

	if(curr_file.is_file_begin)
	{
		*file = curr_file;

		return hel_success;
	}

	return hel_iterate_files(id, file);
}

hel_ret hel_iterate_files(hel_file_id *id, hel_chunk  *file)
{
	hel_ret ret;
	hel_chunk curr_file;
	hel_file_id curr_id;

	if(*id >= NUM_OF_SECTORS)
	{
		return hel_boundaries_err;
	}

	curr_file = *file;
	curr_id = *id;

	while(true)
	{
		ret = hel_iterator(&curr_file, &curr_id);
		if(ret != hel_success)
		{
			return ret;
		}

		if(curr_file.is_file_begin)
		{
			*id = curr_id;
			*file = curr_file;
			return hel_success;
		}
	}

	assert(false);
	return hel_success;
}
