#include "mphalport.h"
#include <tick_macros.h>
#include <platform-timer.hh>

uint64_t MP_HAL_FUNC get_time() {
    StandardClint::init();
    return StandardClint::time();
}

// void MP_HAL_FUNC set_next(uint64_t next_time) {
//     StandardClint::init();
//     StandardClint::setnext(next_time);
// }

// void MP_HAL_FUNC clear() {
//     StandardClient::clear();
// }
