#include "py/obj.h"

#define CLOCK_FREQ_KHZ (50000)
#define MS_TO_CLOCK_CYCLES(x) (x * CLOCK_FREQ_KHZ) // clock 50mHz

extern "C" uint64_t get_time();
