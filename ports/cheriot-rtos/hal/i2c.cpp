#include <stdio.h>
#include <platform-i2c.hh>

extern "C" void i2c_setup(volatile OpenTitanI2c *i2c, uint32_t freq_kHz) {
    i2c->reset_fifos();
    i2c->host_mode_set();
    i2c->speed_set(freq_kHz);
}

extern "C" bool i2c_blocking_read(volatile OpenTitanI2c *i2c, uint8_t addr, uint8_t *buf, uint32_t len) {
    return i2c->blocking_read(addr, buf, len);
}

extern "C" bool i2c_check_success(volatile OpenTitanI2c *i2c) {
    while (!i2c->format_is_empty()) {
    }
    if (i2c->interrupt_is_asserted(OpenTitanI2cInterrupt::Nak)) {
        i2c->interrupt_clear(OpenTitanI2cInterrupt::Nak);
        return false;
    }
    return true;
}

extern "C" bool i2c_blocking_write(volatile OpenTitanI2c *i2c, uint8_t addr, uint8_t *buf, uint32_t len, bool skipStop) {
    i2c->blocking_write(addr, buf, len, skipStop);
    return i2c_check_success(i2c);
}


extern "C" bool i2c_send_address(volatile OpenTitanI2c *i2c, uint8_t addr) {
    i2c->blocking_write_byte(OpenTitanI2c::FormatDataStart | OpenTitanI2c::FormatDataStop | (addr << 1) | 1u);
    return i2c_check_success(i2c);
}
