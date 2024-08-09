#ifndef MPHALPORT_H
#define MPHALPORT_H

#include <compartment.h>

#include "py/obj.h"

#define mp_hal_ticks_ms() (0UL)

#define mp_hal_set_interrupt_char(c) ((void)c)

#define MP_HAL_FUNC __cheri_compartment("mp_hal")

int MP_HAL_FUNC _mp_hal_stdin_rx_chr(void);
void MP_HAL_FUNC _mp_hal_stdout_tx_str(const char *str);
mp_uint_t MP_HAL_FUNC _mp_hal_stdout_tx_strn(const char *str, size_t len);
void MP_HAL_FUNC _mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);


typedef struct OpenTitanI2c open_titan_i2c_t;

void MP_HAL_FUNC _i2c_setup(volatile open_titan_i2c_t *i2c, uint32_t freq_kHz);
bool MP_HAL_FUNC _i2c_send_address(volatile open_titan_i2c_t *i2c, uint8_t addr);
bool MP_HAL_FUNC _i2c_blocking_read(volatile open_titan_i2c_t *i2c, uint8_t addr, uint8_t *buf, uint32_t len);
bool MP_HAL_FUNC _i2c_blocking_write(volatile open_titan_i2c_t *i2c, uint8_t addr, uint8_t *buf, uint32_t len, bool skipStop);


typedef struct OpenTitanUart open_titan_uart_t;

void MP_HAL_FUNC _uart_init(volatile open_titan_uart_t *block, uint32_t baudrate);
uint8_t MP_HAL_FUNC _uart_get_rx_level(volatile open_titan_uart_t *block);
uint8_t MP_HAL_FUNC _uart_get_tx_level(volatile open_titan_uart_t *block);
bool MP_HAL_FUNC _uart_is_readable(volatile open_titan_uart_t *block);
bool MP_HAL_FUNC _uart_is_writable(volatile open_titan_uart_t *block);
bool MP_HAL_FUNC _uart_timeout_read(volatile open_titan_uart_t *block, uint8_t *out, uint32_t timeout_ms);
void MP_HAL_FUNC _uart_blocking_write(volatile open_titan_uart_t *block, uint8_t data);


#ifdef MP_VM_COMP
#define mp_hal_stdin_rx_chr() MP_STATE_THREAD_HACK_SPILL_FOR(_mp_hal_stdin_rx_chr(), int)
#define mp_hal_stdout_tx_str(str) MP_STATE_THREAD_HACK_SPILL_FOR_V(_mp_hal_stdout_tx_str(str))
#define mp_hal_stdout_tx_strn(str, len) MP_STATE_THREAD_HACK_SPILL_FOR(_mp_hal_stdout_tx_strn(str, len), mp_uint_t)
#define mp_hal_stdout_tx_strn_cooked(str, len) MP_STATE_THREAD_HACK_SPILL_FOR_V(_mp_hal_stdout_tx_strn_cooked(str, len))


#define i2c_setup(i2c, freq_kHz) MP_STATE_THREAD_HACK_SPILL_FOR_V(_i2c_setup(i2c, freq_kHz))
#define i2c_send_address(i2c, addr) MP_STATE_THREAD_HACK_SPILL_FOR(_i2c_send_address(i2c, addr), bool)
#define i2c_blocking_read(i2c, addr, buf, len) MP_STATE_THREAD_HACK_SPILL_FOR(_i2c_blocking_read(i2c, addr, buf, len), bool)
#define i2c_blocking_write(i2c, addr, buf, len, skipStop) MP_STATE_THREAD_HACK_SPILL_FOR(_i2c_blocking_write(i2c, addr, buf, len, skipStop), bool)


#define uart_init(block, baudrate) MP_STATE_THREAD_HACK_SPILL_FOR_V(_uart_init(block, baudrate))
#define uart_get_rx_level(block) MP_STATE_THREAD_HACK_SPILL_FOR(_uart_get_rx_level(block), uint8_t)
#define uart_get_tx_level(block) MP_STATE_THREAD_HACK_SPILL_FOR(_uart_get_tx_level(block), uint8_t)
#define uart_is_readable(block) MP_STATE_THREAD_HACK_SPILL_FOR(_uart_is_readable(block), bool)
#define uart_is_writable(block) MP_STATE_THREAD_HACK_SPILL_FOR(_uart_is_writable(block), bool)
#define uart_timeout_read(block, out, timeout_ms) MP_STATE_THREAD_HACK_SPILL_FOR(_uart_timeout_read(block, out, timeout_ms), bool)
#define uart_blocking_write(block, data) MP_STATE_THREAD_HACK_SPILL_FOR_V(_uart_blocking_write(block, data))

#else
#define mp_hal_stdin_rx_chr _mp_hal_stdin_rx_chr
#define mp_hal_stdout_tx_str _mp_hal_stdout_tx_str
#define mp_hal_stdout_tx_strn _mp_hal_stdout_tx_strn
#define mp_hal_stdout_tx_strn_cooked _mp_hal_stdout_tx_strn_cooked


#define i2c_setup _i2c_setup
#define i2c_send_address _i2c_send_address
#define i2c_blocking_read _i2c_blocking_read
#define i2c_blocking_write _i2c_blocking_write


#define uart_init _uart_init
#define uart_get_rx_level _uart_get_rx_level
#define uart_get_tx_level _uart_get_tx_level
#define uart_is_readable _uart_is_readable
#define uart_is_writable _uart_is_writable
#define uart_timeout_read _uart_timeout_read
#define uart_blocking_write _uart_blocking_write
#endif

#endif
