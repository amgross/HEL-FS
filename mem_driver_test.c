
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hel_kernel.h"

static uint8_t *mem_buff = NULL;
static uint32_t mem_size;
static uint32_t sector_size;

extern void fill_rand_buff(uint8_t *buff, size_t len);

void mem_driver_init_test(uint32_t size, uint32_t _sector_size)
{
	mem_size = size;
	sector_size = _sector_size;
	mem_buff = (uint8_t *)malloc(size);
	assert(size > sector_size);
	assert(size % sector_size == 0);
	assert(mem_buff != NULL);

	fill_rand_buff(mem_buff, size);
}

hel_ret mem_driver_init(uint32_t *size, uint32_t *_sector_size)
{
	assert(size != NULL);
	assert(_sector_size != NULL);
	assert(mem_buff != NULL);

	*size = mem_size;
	*_sector_size = sector_size;

	return hel_success;
}

hel_ret mem_driver_close()
{
	return hel_success;
}

hel_ret mem_driver_write(uint32_t v_addr, int size, char *in)
{
	assert(mem_buff != NULL);
	assert((v_addr < mem_size) && (mem_size - v_addr >= size));

	memcpy(mem_buff + v_addr, in, size);

	return hel_success;
}

hel_ret mem_driver_read(uint32_t v_addr, int size, char *out)
{
	assert(mem_buff != NULL);
	assert((v_addr < mem_size) && (mem_size - v_addr >= size));

	memcpy(out, mem_buff + v_addr, size);

	return hel_success;
}