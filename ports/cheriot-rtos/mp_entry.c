#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <compartment.h>

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "shared/runtime/pyexec.h"

/* All compartment entry points return 0 on success and -1 on failure. This is consistent with
 * the RTOS installing a return value of -1 on forced unwind */

#if MICROPY_ENABLE_COMPILER
int __cheri_compartment("mp_vm") mp_exec_str_single(const char *src) {
    MP_STATE_THREAD_HACK_INIT
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_SINGLE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
	return 0;
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
	return -1;
    }
}
int __cheri_compartment("mp_vm") mp_exec_str_file(const char *src) {
    MP_STATE_THREAD_HACK_INIT
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
	return 0;
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
	return -1;
    }
}
int __cheri_compartment("mp_vm") mp_exec_func(const char * func, void * ret, int n_args, const char * sig, ...) {
    if(!func || n_args < 0 || !sig || !*sig) return -1;
    MP_STATE_THREAD_HACK_INIT
    va_list ap;
    mp_obj_t * args = alloca(n_args * sizeof(mp_obj_t));
    va_start(ap, sig);
    for(int i = 0; i < n_args; i++) {
        switch(sig[i + 1]) {
            case 'i': /* int */
                args[i] = mp_obj_new_int(va_arg(ap, int));
		break;
            case 'I': /* uint */
		args[i] = mp_obj_new_int_from_uint(va_arg(ap, unsigned));
		break;
#if MICROPY_PY_BUILTINS_FLOAT
            case 'f': /* float */
		args[i] = mp_obj_new_float_from_f(va_arg(ap, float));
		break;
	    case 'd': /* double */
		args[i] = mp_obj_new_float_from_d(va_arg(ap, double));
		break;
#endif
	    case 's': { /* const char * */
	        const char * s = va_arg(ap, const char *);
		args[i] = s ? mp_obj_new_str(s, strlen(s)) : mp_const_none;
		break;
            }
	    case 'P': /* void* */
	        args[i] = mp_obj_new_cap(va_arg(ap, void *));
	        break;
	    case 'O': { /* TODO -- sealed python object */
		return -1;
	      /*  args[i] = va_arg(ap, mp_obj_t);
		if(!mp_obj_is_obj(args[i])) break;
	        void * ptr = __builtin_cheri_unseal(MP_OBJ_TO_PTR(args[i]), SEALING_CAP());
		if(!__builtin_cheri_tag_get(ptr)) return -1;
		args[i] = MP_OBJ_FROM_PTR(ptr); 
		break; */
            }
	    default:
	        return -1;
	}	
    }
    va_end(ap);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t func_obj = mp_load_global(qstr_find_strn(func, strlen(func)));
        mp_obj_t retval = mp_call_function_n_kw(func_obj, n_args, 0, args);
        nlr_pop();
	switch(*sig) {
	    case 'i':
            case 'I': 
                *(int*)ret = mp_obj_get_int_truncated(retval);
		return 0;
#if MICROPY_PY_BUILTINS_FLOAT
            case 'f': /* float */
		*(float*)ret = mp_obj_get_float_to_f(retval);
		return 0;
	    case 'd': /* double */
		*(double*)ret = mp_obj_get_float_to_d(retval);
		return 0;
#endif
	    case 's': /* const char * */
	        *(const char**)ret = __builtin_cheri_perms_and(mp_obj_str_get_str(retval), ~CHERI_PERM_LOAD);
		return 0;
	    case 'P': /* void* */
	        *(void**)ret = mp_obj_cap_get(retval);
	        return 0;
	    case 'O': { /* TODO -- sealed python object */
	        return -1;
		/*
                if(!__builtin_cheri_tag_get(MP_OBJ_TO_PTR(retval))) {
                    *(mp_obj_t*)ret = retval;
		    return 0;
		}
	        void * tmp = __builtin_cheri_seal(MP_OBJ_TO_PTR(retval), SEALING_CAP());
		if(!__builtin_cheri_tag_get(tmp)) return -1;
		*(mp_obj_t*)ret = MP_OBJ_FROM_PTR(tmp);
		return 0;*/
            }
	    default:
		return -1;
	}
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
	return -1;
    }
}
#endif

#if MICROPY_ENABLE_GC
static char heap[16384]; //[MICROPY_HEAP_SIZE];
#endif

#include <stdio.h>
int __cheri_compartment("mp_vm") mp_vminit(void) {
    MP_STATE_THREAD_HACK_INIT
    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    printf("GC initialised\n");
    #endif
    mp_init();
    printf("Micropython initialised\n");
    return 0;
}

int __cheri_compartment("mp_vm") mp_friendly_repl(void) {
    MP_STATE_THREAD_HACK_INIT
    pyexec_friendly_repl();
    return 0;
}

int __cheri_compartment("mp_vm") mp_exec_frozen_module(const char *name) {
    MP_STATE_THREAD_HACK_INIT
    return pyexec_frozen_module(name, false) ? 0 : -1;
}

int __cheri_compartment("mp_vm") mp_vmexit(void) {
    MP_STATE_THREAD_HACK_INIT
    mp_deinit();
    return 0;
}

enum ErrorRecoveryBehaviour compartment_error_handler(struct ErrorState *frame, size_t mcause, size_t mtval) {
#ifndef NDEBUG
    static const char * const cheri_exc_code[32] = { "NONE", "BOUNDS", "TAG", "SEAL" , "04" , "05" , "06", "07",
	                                             "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
						     "10", "EXEC", "LOAD", "STORE", "14", "STCAP", "STLOC", "17",
						     "SYSREG", "19", "1a", "1b", "1c", "1d", "1e", "1f" };
    static const char * const reg_names[16] = { "zr", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
	                                        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5" };

    if(mcause == 0x1c) {
        printf("CHERI fault (type %s):\n", cheri_exc_code[mtval & 0x1f]);
        printf("\tat %p\n", frame->pcc);
	if(mtval & 0x400) { // S=1, special register
            switch(mtval >> 5) {
		case 0x20: printf("\ton PCC\n"); break;
                case 0x3c: printf("\ton MTCC\n"); break;
                case 0x3d: printf("\ton MTDC\n"); break;
                case 0x3e: printf("\ton MScratchC\n"); break;
                case 0x3f: printf("\ton MEPCC\n"); break;
	        default: printf("on unknown special register %ld\n", (mtval >> 5) & 0x1f); 
	    }
	} else {
	    int errreg = mtval >> 5;
            void * errcap = frame->registers[errreg - 1];
            printf("\ton capability %p [base %lx, len %lx, perms %lx] in register c%s\n", errcap, __builtin_cheri_base_get(errcap), __builtin_cheri_length_get(errcap), __builtin_cheri_perms_get(errcap), reg_names[errreg]);
	}
    }
    else {
        printf("Error (mcause 0x%lx; mtval 0x%lx):\n", mcause, mtval);
        printf("\tat %p\n", frame->pcc);
    }
	printf("\tRegisters:\n");
	for(int i = 0; i < 15; i++) {
	    printf("\t\tc%s : %p [base %lx, len %lx, perms %lx, tag %d]\n", reg_names[i+1], frame->registers[i], __builtin_cheri_base_get(frame->registers[i]), __builtin_cheri_length_get(frame->registers[i]), __builtin_cheri_perms_get(frame->registers[i]), __builtin_cheri_tag_get(frame->registers[i]));
	}
	printf("\tStack view:\n", frame->registers[1]);
	void ** stack = (void**) frame->registers[1];
	if(__builtin_cheri_tag_get(stack)) for(int i = 0; i < 32; i++) {
	    printf("\t\t+%02x : %p [base %lx, len %lx, perms %lx, tag %d]\n", sizeof(void*)*i, stack[i], __builtin_cheri_base_get(stack[i]), __builtin_cheri_length_get(stack[i]), __builtin_cheri_perms_get(stack[i]), __builtin_cheri_tag_get(stack[i]));
	}
    while(1);
#endif
    /* Hack -- assume EBREAK comes from nlr_jump_fail() and unwind accordingly */
    if(mcause == 3) return ForceUnwind;
    
    /* Build exception object
     *
     * TODO we probably want to refine exception types but for now we wrap in OSError
     * - `mcause` will never be greater than 0x1c, so we pack it into the top byte 
     */ 
    mp_obj_t exc = mp_obj_new_exception_arg1(&mp_type_OSError, (mcause << 24) | (mtval & 0xffffff));
    /* XXX -- we cannot simply call mp_raise_xxx() or nlr_raise() because the RTOS
     * will think we are still in the handler and treat future exceptions 
     * as a double-fault condition.
     * Instead, we prepare a 'return' to a call to nlr_jump()
     */
    frame->pcc = &nlr_jump;
    frame->registers[0] = NULL;               /* cra -- nlr_jump never returns */
    frame->registers[9] = MP_OBJ_TO_PTR(exc); /* ca0 -- exception value */
    /* csp, cgp should remain the same
     * all other registers are ignored and overwritten by nlr_jump 
     */
    return InstallContext;
}


#if MICROPY_ENABLE_GC
void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void * dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((size_t)MP_STATE_THREAD(stack_top) - (size_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info(&mp_plat_print);
}
#endif

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

void nlr_jump_fail(void *val) {
#ifndef NDEBUG
    printf("internal error: NLR jump fail\n");
#endif
    /* Trap to error handler */
    __asm__("ebreak");
    printf("!!! Unreachable !!!\n");
    while(1);
}

