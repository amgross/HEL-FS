
#pragma once

#include <stdint.h>

#include "hel_kernel.h"

hel_ret mem_driver_init(HEL_BASE_TYPE *size, HEL_BASE_TYPE *_sector_size);

hel_ret mem_driver_close();

hel_ret mem_driver_write(HEL_BASE_TYPE v_addr, HEL_BASE_TYPE *atomic_write, void **in, HEL_BASE_TYPE* size, HEL_BASE_TYPE buffs_num);

hel_ret mem_driver_read(HEL_BASE_TYPE v_addr, HEL_BASE_TYPE size, void *out);