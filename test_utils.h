
#include <stdint.h>
#include <setjmp.h>

void mem_driver_init_test(uint32_t size, uint32_t sector_size);

typedef enum{
	PD_NONE,
	PD_BEFORE_OERATION_RANDOMLY,
}power_down_option;
extern power_down_option power_down;
extern int power_down_prob;
extern jmp_buf env;
