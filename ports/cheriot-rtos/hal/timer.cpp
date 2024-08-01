#include <platform-timer.hh>

extern "C" uint64_t get_time() {
    StandardClint::init();
    return StandardClint::time();
}

// extern "C" void set_next(uint64_t next_time) {
//     StandardClint::init();
//     StandardClint::setnext(next_time);
// }

// extern "C" void clear() {
//     StandardClient::clear();
// }
