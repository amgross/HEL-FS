
#pragma once

#include <stdint.h>

#include "hel_kernel.h"

hel_ret mem_driver_init(uint32_t *size, uint32_t *_sector_size);

hel_ret mem_driver_write(uint32_t v_addr, int size, char *in);

hel_ret mem_driver_read(uint32_t v_addr, int size, char *out);