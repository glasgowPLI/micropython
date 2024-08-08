#include "py/obj.h"
#include "mphalport.h"

#define CLOCK_FREQ_KHZ (50000)
#define MS_TO_CLOCK_CYCLES(x) (x * CLOCK_FREQ_KHZ) // clock 50mHz

uint64_t MP_HAL_FUNC get_time();
