#include <stdio.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "extmod/modmachine.h"

#include "mphalport.h"

#define I2C_DEFAULT_FREQ (400000)

#define SPI_0_ADDR (0x80200000)
#define SPI_1_ADDR (0x80201000)


typedef struct _machine_i2c_obj_t {
    mp_obj_base_t base;
    volatile open_titan_i2c_t *i2c_block;
} machine_i2c_obj_t;


static void machine_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)MP_OBJ_TO_PTR(self_in);
    const char *name;

    switch ((ptraddr_t)self->i2c_block) {
        case SPI_0_ADDR:
            name = "0";
            break;
        case SPI_1_ADDR:
            name = "1";
            break;
        default:
            name = "Unknown SPI device";
            break;
    }

    mp_printf(print, "I2C(%s)", name);
}


static volatile open_titan_i2c_t *get_i2c_block(uint32_t id) {
    switch (id) {
        case 0:
            return MMIO_CAPABILITY(open_titan_i2c_t, i2c0);
        case 1:
            return MMIO_CAPABILITY(open_titan_i2c_t, i2c1);
        default:
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Unknown i2c device \"%d\""), id);
    }
}

mp_obj_t machine_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_freq, ARG_scl, ARG_sda, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT},
        {MP_QSTR_freq, MP_ARG_INT, {.u_int = I2C_DEFAULT_FREQ}},
        {MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
    };

    // Parse args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args),
        allowed_args, args);

    if (args[ARG_timeout].u_obj != -1) {
        mp_raise_NotImplementedError(
            MP_ERROR_TEXT("explicit choice of timeout is not implemented"));
    }


    if ((args[ARG_scl].u_obj != MP_OBJ_NULL) ||
        (args[ARG_sda].u_obj != MP_OBJ_NULL)) {
        mp_raise_NotImplementedError(
            MP_ERROR_TEXT("explicit choice of sck/sda is not implemented"));
    }

    volatile open_titan_i2c_t *block = get_i2c_block(args[ARG_id].u_int);
    uint32_t freq_kHz = args[ARG_freq].u_int / 1000;

    i2c_setup(block, freq_kHz);

    machine_i2c_obj_t *self = m_new_obj(machine_i2c_obj_t);
    self->i2c_block = block;
    self->base.type = &machine_i2c_type;

    return MP_OBJ_FROM_PTR(self);
}


static int machine_i2c_transfer_single(mp_obj_base_t *self_in, uint16_t addr, size_t len, uint8_t *buf, unsigned int flags) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)MP_OBJ_TO_PTR(self_in);
    bool stop = 0 != (flags & MP_MACHINE_I2C_FLAG_STOP);

    bool success;

    if (len == 0) {
        success = i2c_send_address(self->i2c_block, addr);

    } else if (flags & MP_MACHINE_I2C_FLAG_READ) {
        success = i2c_blocking_read(self->i2c_block, addr, buf, len);

    } else {
        success = i2c_blocking_write(self->i2c_block, addr, buf, len, !stop);
    }
    return success ? len : -5;
}

static const mp_machine_i2c_p_t machine_i2c_p = {
    .transfer = mp_machine_i2c_transfer_adaptor,
    .transfer_single = machine_i2c_transfer_single,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, machine_i2c_make_new,
    print, machine_i2c_print,
    protocol, &machine_i2c_p,
    locals_dict, &mp_machine_i2c_locals_dict
    );
