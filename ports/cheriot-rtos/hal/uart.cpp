#include <platform-uart.hh>

extern uint64_t get_time();

#define CLOCK_FREQ_KHZ (50000)
#define MS_TO_CLOCK_CYCLES(x) (x * CLOCK_FREQ_KHZ) // clock 50mHz

extern "C" uint8_t uart_get_rx_level(volatile OpenTitanUart<115'200> *block) {
    return (block->fifoStatus >> 16) & 0xff;
}

extern "C" uint8_t uart_get_tx_level(volatile OpenTitanUart<115'200> *block) {
    return block->fifoStatus & 0xff;
}

extern "C" bool uart_is_readable(volatile OpenTitanUart<115'200> *block) {
    return block->can_read();
}

extern "C" bool uart_is_writable(volatile OpenTitanUart<115'200> *block) {
    return block->can_write();
}

extern "C" bool uart_read(volatile OpenTitanUart<115'200> *block, uint8_t *out, uint32_t timeout_ms) {
    uint32_t t_end = get_time() + MS_TO_CLOCK_CYCLES(timeout_ms);

    while (!block->can_read()) {
        if (get_time() > t_end) {
            return false;
        }
    }

    *out = block->rData;
    return true;
}

extern "C" void uart_write(volatile OpenTitanUart<115'200> *block, uint8_t data) {
    while (!block->can_write()) {
    }
    block->wData = data;
}

extern "C" void uart_init(volatile OpenTitanUart<115'200> *block, uint32_t baudrate) {
    block->init(baudrate);
}
