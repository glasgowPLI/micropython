#define MICROPY_PY_MACHINE_EXTRA_GLOBALS \
  {MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type)},

static void mp_machine_idle(void) {}