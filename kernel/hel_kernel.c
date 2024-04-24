
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

static uint8_t *used_map;

/*
 * hel_metadata_structure in bits:
 * 0-29 (30 bits):
 * 	if this is not end struct:
 * 		0-14  (15 bits) - next chunk id
 * 		15-29 (15 bits) - current chunk size !in sectors!
 * 	else (this is file end chunk)
 * 		0-29  (30 bits) - current chunk size !in bytes!
 * 30 (1 bit) - is file start
 * 31 (1 bit) - is file end
 */
typedef uint32_t hel_metadata;
static_assert(sizeof(hel_metadata) == ATOMIC_WRITE_SIZE);

#define META_NOT_END_NEXT_MASK				0x00007FFF
#define META_NOT_END_NEXT_OFFSET			0

#define META_NOT_END_SECTORS_SIZE_MASK		0x3FFF8000
#define META_NOT_END_SECTORS_SIZE_OFFSET 	15

#define MAX_SECTORS_NUM ((1 << 15) - 1) // This is for next id and sectors size will always fit in metadata

#define META_END_BYTES_SIZE_MASK			0x3FFFFFFF
#define META_END_BYTES_SIZE_OFFSET			0

#define MAX_BYTES_NUM ((1 << 30) - 1) // This is for bytes size will always fit in metadata

#define META_IS_START_MASK					0x40000000
#define META_IS_START_OFFSET				30

#define META_IS_END_MASK					0x80000000
#define META_IS_END_OFFSET					31

#define META_GETTER(meta, mask, offset) (((meta) & (mask)) >> offset)
#define META_SETTER(meta, mask, offset, val) ((meta) = ((meta) & ~(mask)) | (val) << (offset))

#define META_NOT_END_NEXT_GET(meta)			META_GETTER(meta, META_NOT_END_NEXT_MASK, META_NOT_END_NEXT_OFFSET)
#define META_NOT_END_SECTORS_SIZE_GET(meta)	META_GETTER(meta, META_NOT_END_SECTORS_SIZE_MASK, META_NOT_END_SECTORS_SIZE_OFFSET)
#define META_END_BYTES_SIZE_GET(meta)		META_GETTER(meta, META_END_BYTES_SIZE_MASK, META_END_BYTES_SIZE_OFFSET)
#define META_IS_START_GET(meta)				META_GETTER(meta, META_IS_START_MASK, META_IS_START_OFFSET)
#define META_IS_END_GET(meta)				META_GETTER(meta, META_IS_END_MASK, META_IS_END_OFFSET)

#define META_NOT_END_NEXT_SET(meta, val)			META_SETTER(meta, META_NOT_END_NEXT_MASK, META_NOT_END_NEXT_OFFSET, val)
#define META_NOT_END_SECTORS_SIZE_SET(meta, val)	META_SETTER(meta, META_NOT_END_SECTORS_SIZE_MASK, META_NOT_END_SECTORS_SIZE_OFFSET, val)
#define META_END_BYTES_SIZE_SET(meta, val)			META_SETTER(meta, META_END_BYTES_SIZE_MASK, META_END_BYTES_SIZE_OFFSET, val)
#define META_IS_START_SET(meta, val)				META_SETTER(meta, META_IS_START_MASK, META_IS_START_OFFSET, val)
#define META_IS_END_SET(meta, val)					META_SETTER(meta, META_IS_END_MASK, META_IS_END_OFFSET, val)

#define CHUNK_SIZE_IN_SECTORS(chunk) (!META_IS_END_GET(*chunk) ? META_NOT_END_SECTORS_SIZE_GET(*chunk) : ROUND_UP_DEV(META_END_BYTES_SIZE_GET(*chunk), sector_size))
#define CHUNK_DATA_BYTES(chunk) (META_IS_END_GET(*chunk) ? META_END_BYTES_SIZE_GET(*chunk) - sizeof(hel_metadata) :(META_NOT_END_SECTORS_SIZE_GET(*chunk) * sector_size) - sizeof(hel_metadata))

#define SET_USED_BIT(id) (used_map[(id) / 8] |= (1 << ((id) % 8)))
#define UNSET_USED_BIT(id) (used_map[(id) / 8] &= ~(1 << ((id) % 8)))
#define GET_USED_BIT(id) (used_map[(id) / 8] &  (1 << ((id) % 8)))

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
#define READ_CHUNK_METADATA(id, p_chunk) mem_driver_read((id) * sector_size, sizeof(hel_metadata), (p_chunk))

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
static hel_ret hel_iterator(hel_metadata *curr_chunk, hel_file_id *id)
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

/*
 * @brief internal function for finding next empty chunk.
 * 
 * @param [IN] id - the chunk id to start searching from.
 * @param [OUT] out_id - the first id of chunk, starting from id, which is not part of file.
 * 
 * @return hel_success if found such empty chunk, hel_mem_err if no such empty chunk exist.
 */
static hel_ret hel_find_empty_chunk(hel_file_id id, hel_file_id *out_id)
{
	assert(id <= NUM_OF_SECTORS);

	for(int curr_id = id; curr_id < mem_size / sector_size; curr_id++)
	{
		if(!GET_USED_BIT(curr_id))
		{
			*out_id = curr_id;
			return hel_success;
		}
	}

	return hel_mem_err;
}

/*
 * @brief internal function for signing internally chunk/chain of chunks as free or part of file.
 * 
 * @param [IN] chunk - the metadata of the first chunk need to sign.
 * @param [IN] start_sector_id - the id of the first sector in the chunk.
 * @param [IN] all_chain - if need to sign just current chunk or aso the whole chain it points to.
 * @param [IN] in_use - if the chunk(s) is in use ore free.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
static hel_ret hel_sign_area(hel_metadata *chunk, hel_file_id start_sector_id, bool all_chain, bool in_use)
{
	hel_ret ret;
	uint32_t num_of_sectors = CHUNK_SIZE_IN_SECTORS(chunk);

	while(true)
	{
		for(hel_file_id id = start_sector_id; id < start_sector_id + num_of_sectors; id++)
		{
			if(in_use)
			{
				SET_USED_BIT(id);
			}
			else
			{
				UNSET_USED_BIT(id);
			}
		}

		if(!all_chain || META_IS_END_GET(*chunk))
		{
			break;
		}

		start_sector_id = META_NOT_END_NEXT_GET(*chunk);

		ret = READ_CHUNK_METADATA(start_sector_id, chunk);
		if(ret != hel_success)
		{
			// TODO how to heal from this?
			return ret;
		}

		num_of_sectors = CHUNK_SIZE_IN_SECTORS(chunk);
	}

	return hel_success;
}

/*
 * @brief internal function for counting how many cosecutive free sectors there are, starting from specific id
 * 
 * @param [IN] id - the id of the sector to start from
 * 
 * @return the number of consecutive free sectors
 */
static int hel_count_consecutive_free_sectors(hel_file_id id)
{
	int ret = 0;

	assert(id < NUM_OF_SECTORS);

	for(; id < NUM_OF_SECTORS; id++)
	{
		if(GET_USED_BIT(id))
		{
			return ret;
		}
		else{
			ret++;
		}
	}

	return ret;
}

/*
 * @brief internal function that decides where to create chunks for file.
 *
 * @param [IN]  size - num of data bytes that need space for them.
 * @param [OUT] chunks_arr - array of chunks to write the data to them. It allocated in the function and should be free outside in case of success.
 * @param [OUT] chunks_num - number of chunks in chunks_arr.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 * 
 * @note this function is the main function that can be changed to optimize writes upon needs. currently it uses greedy implementation that chooses the first chunks.
 */
static hel_ret hel_get_chunks_for_file(int size, chunk_data **chunks_arr, int *chunks_num)
{
	hel_file_id new_file_id = 0;
	int empty_sectors;
	chunk_data *chunks_arr_tmp;
	hel_ret ret;

	*chunks_arr = NULL;
	*chunks_num = 0;


	while((ret = hel_find_empty_chunk(new_file_id, &new_file_id)) == hel_success)
	{			
		empty_sectors = hel_count_consecutive_free_sectors(new_file_id);
		chunks_arr_tmp = (chunk_data *)realloc(*chunks_arr, (*chunks_num + 1) * sizeof(chunk_data));
		if(chunks_arr_tmp == NULL)
		{
			free(*chunks_arr);
			return hel_out_of_heap_err;
		}

		*chunks_arr = chunks_arr_tmp;
		(*chunks_arr)[*chunks_num].id = new_file_id;

		int size_in_empty = (empty_sectors * sector_size) - sizeof(hel_metadata);
		if(size_in_empty >= size)
		{ // Last chunk
			(*chunks_arr)[*chunks_num].size = size;
			(*chunks_num)++;
			break;
		}
		else
		{ // not last chunk
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

/*
 * @brief Before writing the actual data, creating/defragmenting chunks.
 *
 * @param [IN] chunks_arr - array of chunks that need to create.
 * @param [IN] chunks_num - number of chunks in chunks_arr.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
static hel_ret hel_organize_chunks_arr(chunk_data *chunks_arr, int chunks_num)
{
	hel_ret ret;

	for(int i = 0; i < chunks_num; i++)
	{
		hel_metadata first_chunk, curr_chunk;
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
		int empty_sectors = hel_count_consecutive_free_sectors(chunks_arr[i].id);
		int needed_sectors = ROUND_UP_DEV(chunks_arr[i].size + sizeof(hel_metadata), sector_size);
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
			hel_metadata end_chunk;
			META_IS_START_SET(end_chunk, 0);
			META_IS_END_SET(end_chunk, 0);
			META_NOT_END_SECTORS_SIZE_SET(end_chunk, empty_sectors - needed_sectors);
			// No need to set next ID, as currently it is not part of file.
			
			mem_driver_write((chunks_arr[i].id + needed_sectors) * sector_size, &end_chunk, NULL, NULL, 0);
		}

#ifdef PROTECT_POWER_LOSS
		if(need_to_update_first || need_to_update_first_and_end)
		{
			// write_first
			hel_metadata first_chunk;
			META_IS_START_SET(first_chunk, 0);
			META_IS_END_SET(first_chunk, 0);
			META_NOT_END_SECTORS_SIZE_SET(first_chunk, needed_sectors);
			// No need to set next ID, as currently it is not part of file.

			mem_driver_write(chunks_arr[i].id * sector_size, &first_chunk, NULL, NULL, 0);
		}
#endif
	}

	return hel_success;
}

/*
 * @brief write chunk with metadata and data.
 *
 * @param [IN] total_size - num of data bytes to write.
 * @param [IN] id - the id of sector to start the chunk from.
 * @param [IN] buff - array of buffers to write the data from.
 * @param [IN] size - array of sizes, of the buffers.
 * @param [IN] is_first - if the chunk is first chunk of file.
 * @param [IN] is_end - if the chunk is last chunk of file.
 * @param [IN] next_id - the id of first sector in next file chunk, if (is_end == true) the value igmnored.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
static hel_ret hel_write_to_chunk(int total_size, hel_file_id id, void **buff, int *size, int num, bool is_first, bool is_end, hel_file_id next_id)
{
	hel_metadata new_file = 0;;
	hel_ret ret;

	if(is_end)
	{
		META_END_BYTES_SIZE_SET(new_file, total_size + sizeof(hel_metadata));
		META_IS_END_SET(new_file, 1);
	}
	else
	{
		int needed_sectors = ROUND_UP_DEV(total_size + sizeof(hel_metadata), sector_size);

		META_NOT_END_SECTORS_SIZE_SET(new_file, needed_sectors);
		META_IS_END_SET(new_file, 0);
		META_NOT_END_NEXT_SET(new_file, next_id);

	}

	META_IS_START_SET(new_file, is_first ? 1: 0);
	
	ret = hel_sign_area(&new_file, id, false, true);
	if(ret != hel_success)
	{
		return ret;
	}

	ret = mem_driver_write(id * sector_size, &new_file, buff, size, num);
	if(ret != hel_success)
	{
		return ret;
	}

	return hel_success;
}

hel_ret hel_init()
{
	hel_ret ret;
	hel_metadata check_chunk;
	hel_file_id curr_id = 0;

	ret = mem_driver_init(&mem_size, &sector_size);
	if(ret != hel_success)
	{
		return ret;
	}

	used_map = (uint8_t *)malloc(ROUND_UP_DEV(mem_size / sector_size, 8));
	if(used_map == NULL)
	{
		return hel_out_of_heap_err;
	}

	memset(used_map, 0, ROUND_UP_DEV(mem_size / sector_size, 8));

	while((ret = hel_find_empty_chunk(curr_id, &curr_id)) == hel_success)
	{ 
		ret = READ_CHUNK_METADATA(curr_id, &check_chunk);
		if(ret != hel_success)
		{
			return ret;
		}

		if(META_IS_START_GET(check_chunk))
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

	free(used_map);
	used_map = NULL;

	ret = mem_driver_close();
	if(ret != hel_success)
	{
		return ret;
	}

	return hel_success;
}

hel_ret hel_format()
{
	hel_metadata first_chunk = 0;
	hel_ret ret;

	ret = mem_driver_init(&mem_size, &sector_size);
	if(ret != hel_success)
	{
		return ret;
	}	

	if(sector_size <= sizeof(hel_metadata))
	{
		return hel_boundaries_err;
	}
	if(mem_size % sector_size != 0)
	{
		return hel_boundaries_err;
	}
	if(mem_size > MAX_BYTES_NUM)
	{
		return hel_boundaries_err;
	}
	if(mem_size / sector_size > MAX_SECTORS_NUM)
	{
		return hel_boundaries_err;
	}
	
	META_NOT_END_SECTORS_SIZE_SET(first_chunk, mem_size / sector_size);
	META_IS_START_SET(first_chunk, 0);
	META_IS_END_SET(first_chunk, 0);

	ret = mem_driver_write(0, &first_chunk, NULL, NULL, 0);
	if(ret != hel_success)
	{
		return ret;
	}

		
	ret = hel_init();
		
	return ret;
}

hel_ret hel_create_and_write(void **in, int *size, int num, hel_file_id *out_id)
{
	int total_size = 0;
	chunk_data *new_chunks_arr;
	int chunks_num;
	hel_ret ret;
	int curr_idx;


	if(NULL == out_id)
	{
		return hel_param_err;
	}

	for(int i = 0; i < num; i++)
	{
		total_size += size[i];
	}
	
	ret = hel_get_chunks_for_file(total_size, &new_chunks_arr, &chunks_num);
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

	// We are writing from end to start (due to power down protection), so pointing to the end.
	curr_idx = num - 1;

	for(int i = chunks_num - 1; i != -1; i--)
	{
		void *buffer_pointer_backup;
		int size_backup;
		int num_of_buffs_to_send = 0;
		int write_size = new_chunks_arr[i].size;
		int write_size_needed = write_size;
		while(true)
		{
			num_of_buffs_to_send++;

			if(write_size_needed > size[curr_idx])
			{
				write_size_needed -= size[curr_idx];
				curr_idx --;
			}
			else 
			{
				size_backup = size[curr_idx] - write_size_needed;
				buffer_pointer_backup = in[curr_idx];
				size[curr_idx]  = write_size_needed;
				in[curr_idx] = (uint8_t *)in[curr_idx] + size_backup;
				break;
			}
		}

		ret = hel_write_to_chunk(write_size, new_chunks_arr[i].id, in + curr_idx, size + curr_idx, num_of_buffs_to_send, i == 0, i == chunks_num - 1, (i == chunks_num - 1)? 0: new_chunks_arr[i + 1].id);
		if(ret != hel_success)
		{
			/// No need to delete something in case of failure, if not all chunks written so nothing really done.

			free(new_chunks_arr);
			return ret;
		}

		if(size_backup == 0)
		{
			curr_idx --;
		}
		else
		{
			size[curr_idx] = size_backup;
			in[curr_idx] =buffer_pointer_backup;
		}
	}
	
	*out_id = new_chunks_arr[0].id;

	free(new_chunks_arr);
	
	return hel_success;
}

hel_ret hel_read(hel_file_id id, void *_out, int begin, int size)
{
	uint8_t *out = _out;
	hel_metadata read_file;
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

	if(!META_IS_START_GET(read_file))
	{
		return hel_not_file_err;
	}

	while(size != 0)
	{
		int chunk_data_bytes = CHUNK_DATA_BYTES(&read_file);
		int begin_offset = HEL_MIN(chunk_data_bytes, begin);
		begin -= begin_offset;
		int read_len = (size > chunk_data_bytes - begin_offset) ? chunk_data_bytes - begin_offset: size;

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
		if(META_IS_END_GET(read_file))
		{
			if(size != 0)
			{
				return hel_boundaries_err;
			}
		}
		else
		{
			id = META_NOT_END_NEXT_GET(read_file);
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
	hel_metadata del_file;
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

	if(!META_IS_START_GET(del_file))
	{
		return hel_not_file_err;
	}

	META_IS_START_SET(del_file, 0);

	ret = hel_sign_area(&del_file, id, true, false);
	if(ret != hel_success)
	{
		return ret;
	}

	ret = mem_driver_write(id * sector_size, &del_file, NULL, NULL, 0);
	if(ret != hel_success)
	{
		return ret;
	}

	return hel_success;
}

hel_ret hel_get_first_file(hel_file_id *id)
{
	hel_ret ret;
	hel_metadata curr_file;

	ret = READ_CHUNK_METADATA(0, &curr_file);
	if(ret != hel_success)
	{
		return ret;
	}
	
	*id = 0;

	if(META_IS_START_GET(curr_file))
	{
		return hel_success;
	}

	return hel_iterate_files(id);
}

hel_ret hel_iterate_files(hel_file_id *id)
{
	hel_ret ret;
	hel_metadata curr_file;
	hel_file_id curr_id;

	if(*id >= NUM_OF_SECTORS)
	{
		return hel_boundaries_err;
	}

	ret = READ_CHUNK_METADATA(*id, &curr_file);
	if(ret != hel_success)
	{
		return ret;
	}

	curr_id = *id;

	while(true)
	{
		ret = hel_iterator(&curr_file, &curr_id);
		if(ret != hel_success)
		{
			if(ret == hel_mem_err)
			{
				return hel_file_not_exist_err;
			}
			else
			{
				return ret;
			}
		}

		if(META_IS_START_GET(curr_file))
		{
			*id = curr_id;
			return hel_success;
		}
	}
}
