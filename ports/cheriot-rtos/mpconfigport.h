#ifndef MP_CONFIG_PORT_H
#define MP_CONFIG_PORT_H

#include <stdint.h>

enum { SPI_MSB_FIRST, SPI_LSB_FIRST };

#define RETURN_TWICE [[gnu::returns_twice]]

// options to control how MicroPython is built

// Use the minimal starting configuration (disables all optional features).
#define MICROPY_CONFIG_ROM_LEVEL \
        (MICROPY_CONFIG_ROM_LEVEL_CORE_FEATURES) // MINIMUM)

// You can disable the built-in MicroPython compiler by setting the following
// config option to 0.  If you do this then you won't get a REPL prompt, but you
// will still be able to execute pre-compiled scripts, compiled with mpy-cross.
#define MICROPY_ENABLE_COMPILER             (1)

#define MICROPY_QSTR_EXTRA_POOL             mp_qstr_frozen_const_pool
#define MICROPY_ENABLE_GC                   (1)
#define MICROPY_HELPER_REPL                 (1)
#define MICROPY_MODULE_FROZEN_MPY           (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT      (1)
#define MICROPY_PY_SYS                      (1)
#define MICROPY_PY_SYS_MAXSIZE              (1)
#define SSIZE_MAX                           INT_MAX
#define MICROPY_PY_SYS_ARGV                 (0)
#define MICROPY_PY_SYS_MODULES              (0)
#define MICROPY_PY_IO                       (0)
#define MICROPY_PY_BUILTINS_MEMORYVIEW      (1)

#define MICROPY_STREAMS_NON_BLOCK           (1)

#define MICROPY_LONGINT_IMPL                (MICROPY_LONGINT_IMPL_MPZ)

#define MICROPY_ALLOC_PATH_MAX              (256)

#define MICROPY_PY_MACHINE                  (1)
#define MICROPY_PY_MACHINE_INCLUDEFILE      "ports/cheriot-rtos/modmachine.c"
#define MICROPY_PY_MACHINE_PIN_MAKE_NEW     mp_pin_make_new
#define MICROPY_PY_MACHINE_SPI              (1)
#define MICROPY_PY_MACHINE_SPI_MSB          (SPI_MSB_FIRST)
#define MICROPY_PY_MACHINE_SPI_LSB          (SPI_LSB_FIRST)
#define MICROPY_PY_MACHINE_I2C              (1)
#define MICROPY_PY_MACHINE_UART             (1)
#define MICROPY_PY_MACHINE_UART_INCLUDEFILE "ports/cheriot-rtos/machine_uart.c"
// Use the minimum headroom in the chunk allocator for parse nodes.
#define MICROPY_ALLOC_PARSE_CHUNK_INIT      (16)

// avoid STORE_LOCAL violations with thread state
#define MICROPY_PY_STATE_THREAD_HACK        (1)

// type definitions for the specific machine

#include <stddef.h>
typedef ssize_t mp_int_t;  // must be pointer size -- or does it?
typedef size_t mp_uint_t;  // must be pointer size -- or does it?

typedef long mp_off_t;

#define MICROPY_HEAP_SIZE (65536)
#define MICROPY_MIN_USE_CHERIOT_A7 (1)
#define alloca(size) __builtin_alloca(size)
static __attribute__((unused)) __attribute__((always_inline)) size_t
strnlen(const char *s, size_t maxlen) {
    int i = 0;
    while (*s++ && ++i < maxlen) {
    }
    return i;
}

#define MICROPY_HW_BOARD_NAME "cheriot"
#define MICROPY_HW_MCU_NAME "unknown-cpu"

#define MP_STATE_PORT MP_STATE_VM

#endif
