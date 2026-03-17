#ifndef MP_ENTRY_H
#define MP_ENTRY_H

#include <stdarg.h>
#include <compartment.h>
#include <token.h>
#include "mpconfigport.h"

typedef struct obj_export_handle obj_export_handle_t;
typedef struct _mp_state_ctx_t mp_state_ctx_t;

#define SCTX CHERI_SEALED(mp_state_ctx_t *)
#define SHANDLE CHERI_SEALED(obj_export_handle_t *)

/* All compartment entry points return 0 on success and -1 on failure. This is consistent with
 * the RTOS installing a return value of -1 on forced unwind */

extern SCTX __cheri_compartment("mp_vm") mp_vminit(size_t heapsize);
extern int __cheri_compartment("mp_vm") mp_vmrestart(SCTX ctx);
extern int __cheri_compartment("mp_vm") mp_vmexit(SCTX ctx);
extern int __cheri_compartment("mp_vm") mp_exec_frozen_module(SCTX ctx, const char *name);
extern int __cheri_compartment("mp_vm") mp_exec_func_v(SCTX ctx, const char *func, void *ret, int n_args, const char *sig, va_list ap);
static inline int mp_exec_func(SCTX ctx, const char *func, void *ret, int n_args, const char *sig, ...) {
    va_list ap;
    va_start(ap, sig);
    int err = mp_exec_func_v(ctx, func, ret, n_args, sig, ap);
    va_end(ap);
    return err;
}
extern int __cheri_compartment("mp_vm") mp_free_obj_handle(SCTX ctx, SHANDLE obj);
#if MICROPY_ENABLE_COMPILER
extern int __cheri_compartment("mp_vm") mp_exec_str_single(SCTX ctx, const char *src);
extern int __cheri_compartment("mp_vm") mp_exec_str_file(SCTX ctx, const char *src);
extern int __cheri_compartment("mp_vm") mp_friendly_repl(SCTX ctx);
extern int __cheri_compartment("mp_vm") mp_raw_repl(SCTX ctx);
extern int __cheri_compartment("mp_vm") mp_var_repl(SCTX ctx);
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
