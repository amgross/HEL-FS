
#include <stdint.h>
#include <setjmp.h>

#include "../kernel/hel_kernel.h"

void mem_driver_init_test(HEL_BASE_TYPE size, HEL_BASE_TYPE sector_size);

typedef enum{
	PD_NONE,
	PD_BEFORE_OERATION_RANDOMLY,
    PD_IN_MIDDLE_RANDOMLY,
}power_down_option;
extern power_down_option power_down;
extern int power_down_prob;
extern jmp_buf env;
