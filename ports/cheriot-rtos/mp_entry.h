#ifndef MP_ENTRY_H
#define MP_ENTRY_H

#include <stdarg.h>
#include <compartment.h>
#include <token.h>
#include "mpconfigport.h"

/* All compartment entry points return 0 on success and -1 on failure. This is consistent with
 * the RTOS installing a return value of -1 on forced unwind */

extern SObj __cheri_compartment("mp_vm") mp_vminit(size_t heapsize);
extern int __cheri_compartment("mp_vm") mp_vmrestart(SObj ctx);
extern int __cheri_compartment("mp_vm") mp_vmexit(SObj ctx);
extern int __cheri_compartment("mp_vm") mp_exec_frozen_module(SObj ctx, const char *name);
extern int __cheri_compartment("mp_vm") mp_exec_func_v(SObj ctx, const char *func, void *ret, int n_args, const char *sig, va_list ap);
static inline int mp_exec_func(SObj ctx, const char *func, void *ret, int n_args, const char *sig, ...) {
    va_list ap;
    va_start(ap, sig);
    int err = mp_exec_func_v(ctx, func, ret, n_args, sig, ap);
    va_end(ap);
    return err;
}
extern int __cheri_compartment("mp_vm") mp_free_obj_handle(SObj ctx, SObj obj);
#if MICROPY_ENABLE_COMPILER
extern int __cheri_compartment("mp_vm") mp_exec_str_single(SObj ctx, const char *src);
extern int __cheri_compartment("mp_vm") mp_exec_str_file(SObj ctx, const char *src);
extern int __cheri_compartment("mp_vm") mp_friendly_repl(SObj ctx);
extern int __cheri_compartment("mp_vm") mp_raw_repl(SObj ctx);
extern int __cheri_compartment("mp_vm") mp_var_repl(SObj ctx);
#endif

typedef union {
    int i;
    unsigned j;
    float f;
    double d;
    void *p;
    const void *q;
} mp_cb_arg_t;

typedef struct {
    mp_cb_arg_t __cheri_callback (*func)(void *data, mp_cb_arg_t *args);
    void *data;
    int n_args;
    const char *sig;
} mp_callback_t;

#endif
