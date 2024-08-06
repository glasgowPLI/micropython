#include <platform-uart.hh>

#include "timer.h"

extern "C" uint8_t uart_get_rx_level(volatile OpenTitanUart<115'200> *block) {
    return block->receive_fifo_level();
}

extern "C" uint8_t uart_get_tx_level(volatile OpenTitanUart<115'200> *block) {
    return block->transmit_fifo_level();
}

extern "C" bool uart_is_readable(volatile OpenTitanUart<115'200> *block) {
    return block->can_read();
}

extern "C" bool uart_is_writable(volatile OpenTitanUart<115'200> *block) {
    return block->can_write();
}

extern "C" bool uart_timeout_read(volatile OpenTitanUart<115'200> *block, uint8_t *out, uint32_t timeout_ms) {
    uint32_t t_end = get_time() + MS_TO_CLOCK_CYCLES(timeout_ms);

    while (!block->can_read()) {
        if (get_time() > t_end) {
            return false;
        }
    }

    *out = block->rData;
    return true;
}

extern "C" void uart_blocking_write(volatile OpenTitanUart<115'200> *block, uint8_t data) {
    block->blocking_write(data);
}

extern "C" void uart_init(volatile OpenTitanUart<115'200> *block, uint32_t baudrate) {
    block->init(baudrate);
}
