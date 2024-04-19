
#pragma once

typedef enum
{
	hel_success,
	hel_mem_err, // Out of memory
	hel_boundaries_err, // Out of boundaries
	hel_param_err, // General parameter error
	hel_not_file_err,
	hel_out_of_heap_err,
}hel_ret;

#define ATOMIC_WRITE_SIZE sizeof(uint32_t)

typedef uint32_t hel_file_id;

/*
 * @brief formats the file system
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 *
 * @note there is no need to run hel_init after running this.
 */
hel_ret hel_format();

/*
 * @brief init the file system data, it should be called before using the file system,
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret hel_init();

/*
 * @brief free all memory allocated at hel_init.
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret hel_close();

/*
 * @brief create file and writes to it.
 *
 * @param [IN] in - buffer to write from.
 * @param [IN] size - number of bytes to write to the file.
 * @param [OUT] out_id - the id of the new file.
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 * 
 * @note the motivation behind adding the write with the create is to reduce number of writes to the memory.
 */
hel_ret hel_create_and_write(void *in, int size, hel_file_id *out_id);

/*
 * @brief read content of file.
 *
 * @param [IN]  id - the id of file.
 * @param [OUT] OUT - buffer to read into it.
 * @param [IN]  begin - index of byte in the file to start read from.
 * @param [IN]  size - number of bytes to read.
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret hel_read(hel_file_id id, void *out, int begin, int size);

/*
 * @brief delete file.
 *
 * @param [IN] id - the id of the fie to delete.
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret hel_delete(hel_file_id id);

/*
 * @brief get the id of the first file in the memory.
 *
 * @param [OUT] id - the id of the file. 
 *
 * @return hel_success upon success, hel_mem_err if there is no file in the system, hel_XXXX_err otherwise.
 * 
 * @note this function is used for starting iteration process with hel_iterate_files.
 */
hel_ret hel_get_first_file(hel_file_id *id);

/*
 * @brief iterator for going over all files.
 *
 * @param [INOUT] id - get some file id, return the id of the next file.
 *
 * @return hel_success upon success, hel_mem_err if id is last file in system, hel_XXXX_err otherwise.
 * 
 * @note to start iteration process use hel_get_first_file.
 * 
 * @note in case of creation/deletion of file, the iteration process should be re-started.
 */
hel_ret hel_iterate_files(hel_file_id *id);
