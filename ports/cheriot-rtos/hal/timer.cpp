#include <cheri.hh>
#include <stdint.h>
#include <utils.hh>

#include "mphalport.h"

class StandardClint : private utils::NoCopyNoMove
{
public:
/**
	 * Initialise the interface.
	 */
static void init() {
    // This needs to be csr_read(mhartid) if we support multicore
    // systems, but we don't plan to any time soon.
    constexpr uint32_t HartId = 0;

    auto setField = [&](auto &field, size_t offset, size_t size) {
            CHERI::Capability capability{MMIO_CAPABILITY(uint32_t, clint)};
            capability.address() += offset;
            capability.bounds() = size;
            field = capability;
        };
    setField(pmtimer, StandardClint::ClintMtime, 2 * sizeof(uint32_t));
    setField(pmtimercmp,
        StandardClint::ClintMtimecmp + HartId * 8,
        2 * sizeof(uint32_t));
}

static uint64_t time() {
    // The timer is little endian, so the high 32 bits are after the low 32
    // bits. We can't do atomic 64-bit loads and so we have to read these
    // separately.
    volatile uint32_t *timerHigh = pmtimer + 1;
    uint32_t timeLow, timeHigh;

    // Read the current time. Loop until the high 32 bits are stable.
    do
    {
        timeHigh = *timerHigh;
        timeLow = *pmtimer;
    } while (timeHigh != *timerHigh);
    return (uint64_t(timeHigh) << 32) | timeLow;
}

private:
#ifdef IBEX_SAFE
/**
	 * The Ibex-SAFE platform doesn't implement a complete CLINT, only the
	 * timer part (which is all that we use).  Its offsets within the CLINT
	 * region are different.
	 */
static constexpr size_t ClintMtime = 0x10;
static constexpr size_t ClintMtimecmp = 0x18;
#else
/**
	 * The standard RISC-V CLINT is a large memory-mapped device and places the
	 * timers a long way in.
	 */
static constexpr size_t ClintMtimecmp = 0x4000U;
static constexpr size_t ClintMtime = 0xbff8U;
#endif

static inline volatile uint32_t *pmtimercmp;
static inline volatile uint32_t *pmtimer;
};

using TimerCore = StandardClint;


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
