

#pragma once

#include <stdint.h>

#include "hel_kernel.h"

/*
 * @brief Init the memory driver for hel-fs usage, return memory sizes.
 *
 * @param [OUT] size - total memory size available in bytes.
 * @param [OUT] sector_size - this is the internal granularity that the file system should use.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 * 
 * @note there are limitations on max size and max sector_size, they will cause hel_format() to fail.
 */
hel_ret mem_driver_init(HEL_BASE_TYPE *size, HEL_BASE_TYPE *sector_size);

/*
 * @brief Informs that the memory driver is not in use anymore by hel-fs (called upon hel-fs close).
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret mem_driver_close();

/*
 * @brief write to memory.
 *
 * @param [IN] v_addr - virtuat address (range 0 - memory size) to start writing to.
 * @param [IN] atomic_write - pointer to HEL_BASE_TYPE variable that should be written to v_addr, in case of NULL this step should be skipped.
 *                              for avoiding corruption upon power down, it should be written atomically and after all other writes of this call done.
 *                              In case it is not NULL, v_address is devidable by sector size.
 * @param [IN] in - array of pointers to buffers, those should be written into the memory addresses one after the other.
 * @param [IN] size - array of sizes of the in array buffers.
 * @param [IN] buffs_num - the size of 'in' and 'size' arrays.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 * 
 * @example if 'v_address' is 64 'atomic_write' is not NULL and 'size' is [32,8], then:
 *              first buffer of 'in' should be copied to virtual memory 68 - 99 (assuming sizeof(HEL_BASE_TYPE)==4)
 *              second buffer of 'in' should be copied to virtual memory 100 - 107 (in time, it is OK that second buffer will be written before the first)
 *              *atomic_write should be write to addresses 64 - 67 (assuming sizeof(HEL_BASE_TYPE)==4). This should be last write for avoiding power down corruption.
 */
hel_ret mem_driver_write(HEL_BASE_TYPE v_addr, HEL_BASE_TYPE *atomic_write, void **in, HEL_BASE_TYPE* size, HEL_BASE_TYPE buffs_num);

/*
 * @brief read from memory
 *
 * @param [IN] v_addr - virtuat address (range 0 - memory size) to start reading from.
 * @param [IN] size - number of bytes to read from memory.
 * @param [OUT] out - buffer to read the data to.
 * 
 * @return hel_success upon success, hel_XXXX_err otherwise.
 */
hel_ret mem_driver_read(HEL_BASE_TYPE v_addr, HEL_BASE_TYPE size, void *out);