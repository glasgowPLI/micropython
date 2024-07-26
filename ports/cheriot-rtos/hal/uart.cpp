#include <platform-uart.hh>

#define UART_TX_FIFO_SIZE (32)

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

extern "C" uint8_t uart_read(volatile OpenTitanUart<115'200> *block) {
    return block->blocking_read();
}

extern "C" void uart_write(volatile OpenTitanUart<115'200> *block, uint8_t data) {
    block->blocking_write(data);
}

extern "C" void uart_init(volatile OpenTitanUart<115'200> *block, uint32_t baudrate) {
    block->init(baudrate);
}
