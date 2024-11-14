
#define curmtimehigh curmtimehigh [[maybe_unused]]
#define curmtime curmtime [[maybe_unused]]
#define curmtimenew curmtimenew [[maybe_unused]]
#include <platform-timer.hh>


extern "C" uint32_t mp_hal_ticks_cpu(void) {
    static int init = 0;
    if(!init) {
        TimerCore::init();
    }
    return TimerCore::time();      
}


