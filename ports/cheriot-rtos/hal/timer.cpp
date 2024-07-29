#include <platform-timer.hh>

uint64_t get_time() {
    StandardClint::init();
    return StandardClint::time();
}
