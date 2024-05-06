
#pragma once

#include "../kernel/hel_kernel.h"

/*
 * Basic application layer that wraps the file system kernel.
 * This wrapper adds functionality of file names.
 * One shouldn't use streightly functions from hel_kernel.h if it uses this wrapper.
 */

#define FILE_NAME_SIZE 8

/*
 * @brief formats the file system
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 *
 * @note there is no need to run hel_init after running this.
 */
hel_ret hel_naming_format();

/*
 * @brief init the file system data, it should be called before using the file system,
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret hel_naming_init();

/*
 * @brief free all memory allocated at hel_init.
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret hel_naming_close();

/*
 * @brief create file and writes to it.
 *
 * @param [IN] name - name of file, size sould be FILE_NAME_SIZE.
 * @param [IN] in - array of buffers to write file data from.
 * @param [IN] size - array of number of bytes to write to the file, each one correspand to the align buffer on the buffers array.
 * @param [IN] num - the number of buffers.
 * @param [OUT] out_id - the new file id.
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 * 
 * @note the motivation behind adding the write with the create is to reduce number of writes to the memory.
 * 
 * @note the motivation behind giving the option to write multiple buffers is to reduce writes overheads.
 */
hel_ret hel_naming_create_and_write(char name[FILE_NAME_SIZE], void **in, HEL_BASE_TYPE *size, HEL_BASE_TYPE num, hel_file_id *out_id);

/*
 * @brief get file id of specific file.
 * 
 * @param [IN] name - name of file, size sould be FILE_NAME_SIZE not including NULL pointer.
 * @param [OUT] id - the file id.
 * 
 * @return hel_success upon success, hel_file_not_exist_err if there is no file with such name, hel_XXXX_err otherwise.
 */
hel_ret hel_naming_get_id(char name[FILE_NAME_SIZE], hel_file_id *id);

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
hel_ret hel_naming_read(hel_file_id id, void *out, HEL_BASE_TYPE begin, HEL_BASE_TYPE size);

/*
 * @brief delete file.
 *
 * @param [IN] id - the id of the fie to delete.
 *
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret hel_naming_delete(hel_file_id id);

