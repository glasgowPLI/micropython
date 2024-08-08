#include "py/stream.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "mphalport.h"

#define MICROPY_PY_MACHINE_UART_CLASS_CONSTANTS


typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
    volatile open_titan_uart_t *uart_block;
    uint32_t timeout_ms;
    uint32_t char_timeout_ms;
} machine_uart_obj_t;


volatile open_titan_uart_t *get_uart_block(uint32_t id) {
    switch (id)
    {
        #if 0
        case 1:
            return MMIO_CAPABILITY(open_titan_uart_t, uart1);
        case 2:
            return MMIO_CAPABILITY(open_titan_uart_t, uart2);
        #endif
        default:
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Unknown UART device \"%d\""), id);
    }
}

static void mp_machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    // machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "UART");
}


static void mp_machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop, ARG_tx, ARG_rx, ARG_cts, ARG_rts,
           ARG_timeout, ARG_timeout_char, ARG_invert, ARG_flow, ARG_txbuf, ARG_rxbuf};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 9600} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_tx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_cts, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rts, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 100} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 100} },
        { MP_QSTR_invert, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_flow, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_txbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 32} },
        { MP_QSTR_rxbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 64} },
    };
    // Parse args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_bits].u_int != 8) {
        mp_raise_ValueError("Bit hard coded to 8");
    }

    if (args[ARG_stop].u_int != 1) {
        mp_raise_ValueError("Stop bits hard coded to 1");
    }

    if ((args[ARG_tx].u_obj != MP_OBJ_NULL) ||
        (args[ARG_rx].u_obj != MP_OBJ_NULL) ||
        (args[ARG_cts].u_obj != MP_OBJ_NULL) ||
        (args[ARG_rts].u_obj != MP_OBJ_NULL)) {
        mp_raise_NotImplementedError(
            MP_ERROR_TEXT("explicit choice of tx/rx/rts/cts pins is not implemented"));
    }

    if (args[ARG_txbuf].u_int != 32) {
        mp_raise_ValueError("tx buffer is 32 bytes long");
    }

    if (args[ARG_rxbuf].u_int != 64) {
        mp_raise_ValueError("rx buffer is 64 bytes long");
    }

    if (args[ARG_invert].u_int != -1) {
        mp_raise_NotImplementedError("UART invert not implemented");
    }

    if (args[ARG_flow].u_int != -1) {
        mp_raise_NotImplementedError("UART flow not implemented");
    }

    // uint32_t parity = 0;
    if (args[ARG_parity].u_obj != MP_OBJ_NULL) {
        mp_raise_NotImplementedError("UART parity not implemented");
        // parity = mp_obj_get_int(args[ARG_parity].u_obj);
    }

    self->timeout_ms = args[ARG_timeout].u_int;
    self->char_timeout_ms = args[ARG_timeout_char].u_int;

    uint32_t baudrate = args[ARG_baudrate].u_int;
    uart_init(self->uart_block, baudrate);
}


static mp_obj_t mp_machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    int id = mp_obj_get_int(args[0]);
    volatile open_titan_uart_t *block = get_uart_block(id);

    machine_uart_obj_t *self = m_new_obj(machine_uart_obj_t);
    self->base.type = &machine_uart_type;
    self->uart_block = block;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}


static void mp_machine_uart_deinit(machine_uart_obj_t *self) {

}

static mp_int_t mp_machine_uart_any(machine_uart_obj_t *self) {
    return uart_get_rx_level(self->uart_block);
}

static bool mp_machine_uart_txdone(machine_uart_obj_t *self) {
    return uart_get_tx_level(self->uart_block) == 0;
}

static mp_uint_t mp_machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t *dest = (uint8_t *)buf_in;

    for (size_t i = 0; i < size; i++) {
        uint32_t timeout = (i == 0? self->timeout_ms:self->char_timeout_ms);
        if (!uart_timeout_read(self->uart_block, &dest[i], timeout)) {
            for (size_t x = 0; x < size; x++) {
            }
            if (i <= 0) {
                *errcode = MP_EAGAIN;
                return MP_STREAM_ERROR;
            } else {
                return i;
            }
        }

    }
    return size;
}


static mp_uint_t mp_machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t *data = (uint8_t *)buf_in;

    for (size_t i = 0; i < size; i++) {
        uart_blocking_write(self->uart_block, data[i]);
    }

    return size;
}


static mp_uint_t mp_machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t ret;

    switch (request) {
        case MP_STREAM_POLL: {
            uintptr_t flags = arg;
            ret = 0;
            if ((flags & MP_STREAM_POLL_RD) && uart_is_readable(self->uart_block)) {
                ret |= MP_STREAM_POLL_RD;
            }
            if ((flags & MP_STREAM_POLL_WR) && uart_is_writable(self->uart_block)) {
                ret |= MP_STREAM_POLL_WR;
            }
            break;
        }
        // case MP_STREAM_FLUSH:
        //     break;

        default: {
            *errcode = MP_EINVAL;
            ret = MP_STREAM_ERROR;
            break;
        }
    }
    return ret;
}
