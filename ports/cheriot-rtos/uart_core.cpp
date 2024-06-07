#include <compartment.h>
#include <platform-uart.hh>

#include "mpconfigport.h"

/*
 * Core UART functions to implement for a port
 */

// Receive single character
extern "C" int mp_hal_stdin_rx_chr(void) {
    auto uart = MMIO_CAPABILITY(Uart, uart);
    return uart->blocking_read();
}

// Send string of given length
extern "C" mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    auto uart = MMIO_CAPABILITY(Uart, uart);
    mp_uint_t ret = len;
    while(len--) {
        uart->blocking_write(*str++);
    }
    return ret;
}
