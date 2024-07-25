#include <string.h>

#include "extmod/modmachine.h"
#include "py/obj.h"
#include "py/runtime.h"

typedef struct {
    uint32_t output;
    uint32_t input;
    uint32_t debounced_input;
    uint32_t debounce_threshold;
} gpio_block_t;

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    volatile gpio_block_t *port;
    uint32_t pin;
    uint32_t mode;
} machine_pin_obj_t;

enum {
    MACHINE_PIN_MODE_IN = 0,
    MACHINE_PIN_MODE_OUT = 1,
    MACHINE_PIN_MODE_OPEN_DRAIN = 2,
    // MACHINE_PIN_MODE_ANALOG  = 3
};

#define GET_BUFFER(self) ((self->port->output >> self->pin) & 1)

static uint32_t set_pin_value(uint32_t block, uint32_t pin, bool value) {
    if (value) {
        return block | (1 << pin);
    } else {
        return block & ~(1 << pin);
    }
}

static uint32_t set_pin_enable(uint32_t block, uint32_t pin, bool value) {
    if (value) {
        return block | (1 << (pin + 16));
    } else {
        return block & ~(1 << (pin + 16));
    }
}

volatile gpio_block_t *get_port(const char *drv_name) {
    // !! MMIO_CAPABILITY names were custom hard-coded into the boards/sonata.json
    // !! file since the headers are currently out of date.
    switch (drv_name[0]) {
        case 'g':
            if (strcmp(drv_name, "gpio") == 0) {
                return MMIO_CAPABILITY(gpio_block_t, gpio);
            }
        #if 0
        case 'r':
            if (strcmp(drv_name, "rpi") == 0) {
                return MMIO_CAPABILITY(gpio_block_t, rpi);
            }
        case 'a':
            if (strcmp(drv_name, "arduino") == 0) {
                return MMIO_CAPABILITY(gpio_block_t, arduino);
            }
        case 'p':
            if (strcmp(drv_name, "pmod") == 0) {
                return MMIO_CAPABILITY(gpio_block_t, pmod);
            }
        #endif
        default:
            mp_raise_msg_varg(&mp_type_ValueError,
                MP_ERROR_TEXT("Unknown port \"%s\""), drv_name);
    }
}

static void machine_pin_obj_init_helper(machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_value };

    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_INT},
        {MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}}
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args),
        allowed_args, args);

    self->mode = args[ARG_mode].u_int;

    uint32_t init = GET_BUFFER(self);
    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
        init = mp_obj_is_true(args[ARG_value].u_obj);
    }

    bool enabled = (self->mode == MACHINE_PIN_MODE_OUT) ||
        (self->mode == MACHINE_PIN_MODE_OPEN_DRAIN && !init);

    uint32_t block = self->port->output;
    block = set_pin_enable(block, self->pin, enabled);
    block = set_pin_value(block, self->pin, init);
    self->port->output = block;
}

// constructor Pin(id, mode, *, value)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    if (!mp_obj_is_type(args[0], &mp_type_tuple)) {
        mp_raise_ValueError(
            MP_ERROR_TEXT("Pin id must be a tuple of (\"name\", pin#)"));
    }

    mp_obj_t *pin_id;
    mp_obj_get_array_fixed_n(args[0], 2, &pin_id);

    const char *drv_name = mp_obj_str_get_str(pin_id[0]);
    int wanted_pin = mp_obj_get_int(pin_id[1]);

    volatile gpio_block_t *wanted_port = get_port(drv_name);

    machine_pin_obj_t *pin = m_new_obj(machine_pin_obj_t);
    pin->base.type = &machine_pin_type;
    pin->port = wanted_port;
    pin->pin = wanted_pin;

    if (n_args > 1 || n_kw > 0) {
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_pin_obj_init_helper(pin, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(pin);
}

// Pin.init(mode, *, value)
static mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    if (mp_obj_is_type(args[0], &machine_pin_type)) {
        machine_pin_obj_init_helper(
            MP_OBJ_TO_PTR(args[0]), n_args - 1, args + 1, kw_args);
        return args[0];
    } else {
        mp_raise_TypeError(
            MP_ERROR_TEXT("Cannot call Pin.init on object that is not Pin"));
    }
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

static mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);

    switch (request) {
        case MP_PIN_READ: {
            if (self->mode == MACHINE_PIN_MODE_OUT) {
                return GET_BUFFER(self);
            }

            return (self->port->input >> self->pin) & 1;
        }
        case MP_PIN_WRITE: {
            uint32_t block = set_pin_value(self->port->output, self->pin, (bool)arg);
            if (self->mode == MACHINE_PIN_MODE_OPEN_DRAIN) {
                block = set_pin_enable(block, self->pin, !(bool)arg);
            }
            self->port->output = block;
            return 0;
        }
    }
    return -1;
}

static mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    if (n_args == 0) {
        return MP_OBJ_NEW_SMALL_INT(mp_virtual_pin_read(self_in));
    } else {
        mp_virtual_pin_write(self_in, mp_obj_is_true(args[0]));
        return mp_const_none;
    }
}


static mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);


static mp_obj_t machine_pin_off(mp_obj_t self_in) {
    mp_virtual_pin_write(self_in, 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);


static mp_obj_t machine_pin_on(mp_obj_t self_in) {
    mp_virtual_pin_write(self_in, 1);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

// print(pin)
static void machine_pin_print(const mp_print_t *print, mp_obj_t self_in,
    mp_print_kind_t kind) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const char *mode_str;

    switch (self->mode) {
        case MACHINE_PIN_MODE_IN:
            mode_str = "IN";
            break;
        case MACHINE_PIN_MODE_OUT:
            mode_str = "OUT";
            break;
        case MACHINE_PIN_MODE_OPEN_DRAIN:
            mode_str = "OPEN DRAIN";
            break;
        default:
            mode_str = "Unknown mode";
    }

    const char *name_str;

    switch ((ptraddr_t)self->port) {
        case 0x80000000:
            name_str = "gpio";
        case 0x80006000:
            name_str = "rpi";
        case 0x80007000:
            name_str = "arduino";
        case 0x80008000:
            name_str = "pmod";
        default:
            name_str = "Unknown port";
    }

    mp_printf(print, "Pin(%s(%d) mode=%s)", name_str, self->pin, mode_str);
}


static const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_low), MP_ROM_PTR(&machine_pin_off_obj)},
    {MP_ROM_QSTR(MP_QSTR_high), MP_ROM_PTR(&machine_pin_on_obj)},
    {MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&machine_pin_off_obj)},
    {MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&machine_pin_on_obj)},

    // class constants
    {MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(MACHINE_PIN_MODE_IN)},
    {MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(MACHINE_PIN_MODE_OUT)},
    {MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(MACHINE_PIN_MODE_OPEN_DRAIN)},
};
static MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

static const mp_pin_p_t pin_pin_p = {
    .ioctl = pin_ioctl,
};

MP_DEFINE_CONST_OBJ_TYPE(machine_pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, mp_pin_make_new,
    print, machine_pin_print,
    call, machine_pin_call,
    protocol, &pin_pin_p,
    locals_dict, &machine_pin_locals_dict);
