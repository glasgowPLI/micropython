#include <compartment.h>
#include "mpconfigport.h"

/* All compartment entry points return 0 on success and -1 on failure. This is consistent with
 * the RTOS installing a return value of -1 on forced unwind */

#if MICROPY_ENABLE_COMPILER
extern int __cheri_compartment("mp_vm") mp_exec_str_single(const char *src);
extern int __cheri_compartment("mp_vm") mp_exec_str_file(const char *src);
extern int __cheri_compartment("mp_vm") mp_friendly_repl(void);
#endif
extern int __cheri_compartment("mp_vm") mp_vminit(void);
extern int __cheri_compartment("mp_vm") mp_exec_frozen_module(const char *name);
extern int __cheri_compartment("mp_vm") mp_exec_func(const char * func, void * ret, int n_args, const char * sig, ...);
extern int __cheri_compartment("mp_vm") mp_vmexit(void);

