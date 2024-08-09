#include <platform-uart.hh>

#include "mphalport.h"
#include "timer.h"

uint8_t MP_HAL_FUNC uart_get_rx_level(volatile OpenTitanUart *block) {
    return block->receive_fifo_level();
}

uint8_t MP_HAL_FUNC uart_get_tx_level(volatile OpenTitanUart *block) {
    return block->transmit_fifo_level();
}

bool MP_HAL_FUNC uart_is_readable(volatile OpenTitanUart *block) {
    return block->can_read();
}

bool MP_HAL_FUNC uart_is_writable(volatile OpenTitanUart *block) {
    return block->can_write();
}

bool MP_HAL_FUNC uart_timeout_read(volatile OpenTitanUart *block, uint8_t *out, uint32_t timeout_ms) {
    uint32_t t_end = get_time() + MS_TO_CLOCK_CYCLES(timeout_ms);

    while (!block->can_read()) {
        if (get_time() > t_end) {
            return false;
        }
    }

    *out = block->readData;
    return true;
}

void MP_HAL_FUNC uart_blocking_write(volatile OpenTitanUart *block, uint8_t data) {
    block->blocking_write(data);
}

void MP_HAL_FUNC uart_init(volatile OpenTitanUart *block, uint32_t baudrate) {
    block->init(baudrate);
}
