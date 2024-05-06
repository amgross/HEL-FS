
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdbool.h>
#include <math.h>


#include "../kernel/hel_kernel.h"
#include "test_utils.h"

static uint8_t *mem_buff = NULL;
static HEL_BASE_TYPE mem_size;
static HEL_BASE_TYPE sector_size;

#define HEL_MIN(x, y) ((x > y) ? y: x)

jmp_buf env;
power_down_option power_down = PD_NONE;
int power_down_prob = 0;

extern void fill_rand_buff(uint8_t *buff, size_t len);

static bool decide_if_power_down(HEL_BASE_TYPE *size, HEL_BASE_TYPE num)
{
	switch(power_down)
	{
		case PD_NONE:
		{
			return false;
		}
		case PD_BEFORE_OERATION_RANDOMLY:
		{
			if((rand() % power_down_prob) == 0)
			{
				longjmp(env, 0);
				assert(false);
			}
			else
			{
				return false;
			}
		}
		case PD_IN_MIDDLE_RANDOMLY:
		{
			if((rand() % power_down_prob) == 0)
			{
				HEL_BASE_TYPE total_size = 0;
				for(HEL_BASE_TYPE i = 0; i < num; i++)
				{
					total_size += size[i];
				}

				if(total_size != 0)
				{
					total_size = rand() % total_size;
				}
				
				for(HEL_BASE_TYPE i = 0; i < num; i++)
				{
					size[i] = HEL_MIN(size[i], total_size); // There is no problem to change it as anyway we about to power down
					total_size -= size[i];
				}

				return true;
			}
			else
			{
				return false;
			}
		}
		default:
		{
			assert(false);
		}
	}
	
	return false;
}

void mem_driver_init_test(HEL_BASE_TYPE size, HEL_BASE_TYPE _sector_size)
{
	mem_size = size;
	sector_size = _sector_size;
	mem_buff = (uint8_t *)malloc(size);
	assert(size >= sector_size);
	assert(size % sector_size == 0);
	assert(mem_buff != NULL);

	fill_rand_buff(mem_buff, size);
}

hel_ret mem_driver_init(HEL_BASE_TYPE *size, HEL_BASE_TYPE *_sector_size)
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

hel_ret mem_driver_write(HEL_BASE_TYPE v_addr, HEL_BASE_TYPE *atomic_write, void **in, HEL_BASE_TYPE* size, HEL_BASE_TYPE buffs_num)
{
	assert(mem_buff != NULL);

	// This is for writing the metadata in the end atomically
	HEL_BASE_TYPE orig_v_addr = v_addr;
	if(atomic_write != NULL)
	{
		v_addr += ATOMIC_WRITE_SIZE;
	}

	bool down = decide_if_power_down(size, buffs_num);

	for(HEL_BASE_TYPE i = 0; i < buffs_num; i++)
	{
		HEL_BASE_TYPE curr_size = size[i];
		assert((v_addr < mem_size) && (mem_size - v_addr >= size[i]));

		memcpy(mem_buff + v_addr, in[i], curr_size);

		v_addr += size[i];
	}

	if(down)
	{
		longjmp(env, 0);
	}

	if(atomic_write != NULL)
	{
		memcpy(mem_buff + orig_v_addr, atomic_write, ATOMIC_WRITE_SIZE);
	}


	return hel_success;
}

hel_ret mem_driver_read(HEL_BASE_TYPE v_addr, HEL_BASE_TYPE size, void *_out)
{
	uint8_t *out = _out;
	assert(mem_buff != NULL);
	assert((v_addr < mem_size) && (mem_size - v_addr >= size));

	memcpy(out, mem_buff + v_addr, size);

	return hel_success;
}