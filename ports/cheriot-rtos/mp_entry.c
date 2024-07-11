#define MALLOC_QUOTA 65536

#include "mp_entry.h"

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/misc.h"
#include "shared/runtime/pyexec.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>


static SKey mp_ctx_key = INVALID_SKEY;

/* We maintain a linked list of the objects we export to ensure they don't get garbage-collected */
typedef struct obj_export_handle {
    mp_obj_t obj;
    struct obj_export_handle *next, **prevnext;
} obj_export_handle_t;

MP_REGISTER_ROOT_POINTER(struct SKeyStruct * obj_key);
MP_REGISTER_ROOT_POINTER(struct obj_export_handle * obj_export_head);

/* All compartment entry points return 0 on success and -1 on failure. This is consistent with
 * the RTOS installing a return value of -1 on forced unwind */

#if MICROPY_ENABLE_COMPILER
int __cheri_compartment("mp_vm") mp_exec_str_single(SObj ctx, const char *src) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
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
int __cheri_compartment("mp_vm") mp_exec_str_file(SObj ctx, const char *src) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
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

int __cheri_compartment("mp_vm") mp_free_obj_handle(SObj ctx, SObj obj) {
    obj_export_handle_t * handle = token_obj_unseal(MP_STATE_VM(obj_key), obj);
    if(!handle) return -1;
    if(handle->next) handle->next->prevnext = handle->prevnext;
    *handle->prevnext = handle->next;
    return token_obj_destroy(MALLOC_CAPABILITY, MP_STATE_VM(obj_key), obj);
}


static int mp_obj_to_cobj(void * ret, char type, mp_obj_t in) {
    switch(type) {
        case 'i':
        case 'I': 
            *(int*)ret = mp_obj_get_int_truncated(in);
            return 0;
#if MICROPY_PY_BUILTINS_FLOAT
        case 'f': /* float */
   	    *(float*)ret = mp_obj_get_float_to_f(in);
	    return 0;
	case 'd': /* double */
	    *(double*)ret = mp_obj_get_float_to_d(in);
	    return 0;
#endif
	case 's': /* const char * */
	    *(const char**)ret = __builtin_cheri_perms_and(mp_obj_str_get_str(in), ~CHERI_PERM_STORE);
	    return 0;
	case 'P': /* void* */
	    *(void**)ret = mp_obj_cap_get(in);
	    return 0;
	case 'O': { /* TODO -- sealed python object */
	    obj_export_handle_t * obj;
	    *(SObj*)ret = token_sealed_unsealed_alloc(&((Timeout){ .elapsed = 0 , .remaining = UnlimitedTimeout }), MALLOC_CAPABILITY, MP_STATE_VM(obj_key), sizeof(obj_export_handle_t), (void**)&obj);
	    obj->obj = in;
            obj->next = MP_STATE_VM(obj_export_head);
	    obj->prevnext = &MP_STATE_VM(obj_export_head);
	    MP_STATE_VM(obj_export_head)->prevnext = &obj->next;
	    MP_STATE_VM(obj_export_head) = obj;
	    return 0;
	}
	default:
	    return -1;
    }
}

typedef struct {
    mp_obj_base_t base;
    mp_cb_arg_t __cheri_callback (*func)(void * data, mp_cb_arg_t * args);
    void * data;
    int n_args;
    char sig[];
} mp_obj_ext_callback_t;

static mp_obj_t ext_callback_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ext_callback,
    MP_QSTR_ext_callback,
    MP_TYPE_FLAG_NONE,
    call, ext_callback_call);

int __cheri_compartment("mp_vm") mp_exec_func_v(SObj ctx, const char * func, void * ret, int n_args, const char * sig, va_list ap) {
    if(!func || n_args < 0 || !sig || !*sig) return -1;
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
    mp_obj_t * args = alloca(n_args * sizeof(mp_obj_t));
    for(int i = 0; i < n_args; i++) {
        switch(sig[i + 1]) {
            case 'i': /* int -> py`int` */
                args[i] = mp_obj_new_int(va_arg(ap, int));
		break;
            case 'I': /* uint -> py`int` */
		args[i] = mp_obj_new_int_from_uint(va_arg(ap, unsigned));
		break;
#if MICROPY_PY_BUILTINS_FLOAT
            case 'f': /* float -> py`float` */
		args[i] = mp_obj_new_float_from_f(va_arg(ap, float));
		break;
	    case 'd': /* double -> py`float` */
		args[i] = mp_obj_new_float_from_d(va_arg(ap, double));
		break;
#endif
	    case 's': { /* const char * -> py`str` (or None) */
	        const char * s = va_arg(ap, const char *);
		args[i] = s ? mp_obj_new_str(s, strlen(s)) : mp_const_none;
		break;
            }
	    case 'P': /* Arbitrary pointer -> py `cap` */
	        args[i] = mp_obj_new_cap(va_arg(ap, void *));
	        break;
	    case 'O': { /* Sealed python object */
		obj_export_handle_t * handle = token_obj_unseal(MP_STATE_VM(obj_key), va_arg(ap, SObj));
		if(!handle) return -1;
		args[i] = handle->obj; 
		break;
            }
            case 'C': { /* Cross-compartment callback */
	        const mp_callback_t cba = va_arg(ap, mp_callback_t);
		mp_obj_ext_callback_t * cb = m_new_obj_var(mp_obj_ext_callback_t, sig, char, cba.n_args + 1);
	        cb->base.type = &mp_type_ext_callback;
                cb->func = cba.func;
		cb->data = cba.data;
		cb->n_args = cba.n_args;
		memcpy(&cb->sig, cba.sig, cba.n_args + 1);
	        args[i] = MP_OBJ_FROM_PTR(cb);
		break;
            }
	    default:
	        return -1;
	}	
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t func_obj = mp_load_global(qstr_find_strn(func, strlen(func)));
        mp_obj_t retval = mp_call_function_n_kw(func_obj, n_args, 0, args);
        nlr_pop();
	return (*sig == 'v') ? 0 : mp_obj_to_cobj(ret, *sig, retval); 
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
	return -1;
    }
}

static mp_obj_t ext_callback_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_obj_ext_callback_t *self = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, self->n_args, self->n_args, false);

    mp_cb_arg_t * cargs = m_new0(mp_cb_arg_t, n_args);
    for(int i = 0; i < n_args; i++) {
	if(mp_obj_to_cobj(&cargs[i], self->sig[i+1], args[i])) {
            m_del(mp_cb_arg_t, cargs, n_args);
	    mp_raise_TypeError(MP_ERROR_TEXT("Bad callback signature"));
	}
    }

    mp_cb_arg_t ret = MP_STATE_THREAD_HACK_SPILL_FOR((*self->func)(self->data, cargs), mp_cb_arg_t);
    m_del(mp_cb_arg_t, cargs, n_args);

    switch(*self->sig) {
        case 'i': /* int -> py`int` */
            return mp_obj_new_int(ret.i);
	case 'I': /* uint -> py`int` */
	    return mp_obj_new_int_from_uint(ret.j);
#if MICROPY_PY_BUILTINS_FLOAT
	case 'f': /* float -> py`float` */
	    return mp_obj_new_float_from_f(ret.f);
	case 'd': /* double -> py`float` */
	    return mp_obj_new_float_from_d(ret.d);
#endif
	case 's': /* const char * -> py`str` (or None) */
   	    return ret.q ? mp_obj_new_str(ret.q, strlen(ret.q)) : mp_const_none;
    	case 'P': /* Arbitrary pointer -> py `cap` */
  	    return mp_obj_new_cap(ret.p);
	case 'O': { /* Sealed python object */
	    obj_export_handle_t * handle = token_obj_unseal(MP_STATE_VM(obj_key), ret.p);
	    return handle ? handle->obj : mp_const_none; 
	}
	case 'v':
	    return mp_const_none;
	default:
	    mp_raise_TypeError(MP_ERROR_TEXT("Bad callback signature"));
    }	
}

#endif

#include <stdio.h>
SObj __cheri_compartment("mp_vm") mp_vminit(size_t heapsize) {
    if(mp_ctx_key == INVALID_SKEY) mp_ctx_key = token_key_new();
    void * ctx;
    SObj vm_handle = token_sealed_unsealed_alloc(&((Timeout){ .elapsed = 0 , .remaining = UnlimitedTimeout }), MALLOC_CAPABILITY, mp_ctx_key, sizeof(mp_state_ctx_t), &ctx);
    if(!ctx || !vm_handle) {
        printf("Failed to init micropython VM -- context allocation failed\n");
	return INVALID_SOBJ;
    }
    char * heap = malloc(heapsize);
    if(!heap) {
        printf("Failed to init micropython VM -- heap allocation failed\n");
	token_obj_destroy(MALLOC_CAPABILITY, mp_ctx_key, vm_handle);
	return INVALID_SOBJ;
    }
    MP_STATE_THREAD_HACK_INIT(ctx)
    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + heapsize);
    printf("GC initialised\n");
    #endif
    mp_init();
    printf("Micropython initialised\n");
    pyexec_frozen_module("_boot.py", false);
    return vm_handle;
}

int __cheri_compartment("mp_vm") mp_vmrestart(SObj ctx) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
    mp_deinit();
    void * heap = MP_STATE_MEM(area.gc_alloc_table_start);
    memset(&mp_state_ctx, 0, sizeof(mp_state_ctx_t));
    if(__builtin_cheri_tag_get(heap)) {
        #if MICROPY_ENABLE_GC
        gc_init(heap, heap + __builtin_cheri_length_get(heap));
        printf("GC reinitialised\n");
        #endif
        mp_init();
        printf("Micropython reinitialised\n");
	return 0;
    } else {
        token_obj_destroy(MALLOC_CAPABILITY, mp_ctx_key, ctx);
	return -1;
    }
}

int __cheri_compartment("mp_vm") mp_raw_repl(SObj ctx) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
    return pyexec_raw_repl();
}

int __cheri_compartment("mp_vm") mp_friendly_repl(SObj ctx) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
    return pyexec_friendly_repl();
}

int __cheri_compartment("mp_vm") mp_var_repl(SObj ctx) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
    int ret = pyexec_file_if_exists("boot.py") & PYEXEC_FORCED_EXIT;
    while(!ret) {
        ret = (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) ? pyexec_raw_repl() : pyexec_friendly_repl(); 
    }
    return ret;
}

int __cheri_compartment("mp_vm") mp_exec_frozen_module(SObj ctx, const char *name) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
    return pyexec_frozen_module(name, false) ? 0 : -1;
}

int __cheri_compartment("mp_vm") mp_vmexit(SObj ctx) {
    MP_STATE_THREAD_HACK_INIT(token_obj_unseal(mp_ctx_key, ctx))
    printf("Exiting Micropython VM"); 
    mp_deinit();
    int err = free(MP_STATE_MEM(area.gc_alloc_table_start));
    err |= token_obj_destroy(MALLOC_CAPABILITY, mp_ctx_key, ctx);
    if(!err) return 0;
    printf("Error: mp_vmexit() failed with code %d", err);
    return -1;
}

enum ErrorRecoveryBehaviour compartment_error_handler(struct ErrorState *frame, size_t mcause, size_t mtval) {
//#ifndef NDEBUG
    static const char * const cheri_exc_code[32] = { "UNWIND", "BOUNDS", "TAG", "SEAL" , "04" , "05" , "06", "07",
	                                             "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
						     "10", "EXEC", "LOAD", "STORE", "14", "STCAP", "STLOC", "17",
						     "SYSREG", "19", "1a", "1b", "1c", "1d", "1e", "1f" };
    static const char * const reg_names[16] = { "zr", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
	                                        "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5" };

    if(mcause == 0x1c) {
        printf("CHERI fault (type %s):\n", cheri_exc_code[mtval & 0x1f]);
        printf("\tat %p in compartment 'mp_vm'\n", frame->pcc);
	if(mtval & 0x400) { // S=1, special register
            switch(mtval >> 5) {
		case 0x20: printf("\ton PCC\n"); break;
                case 0x3c: printf("\ton MTCC\n"); break;
                case 0x3d: printf("\ton MTDC\n"); break;
                case 0x3e: printf("\ton MScratchC\n"); break;
                case 0x3f: printf("\ton MEPCC\n"); break;
	        default: printf("on unknown special register %ld\n", (mtval >> 5) & 0x1f); 
	    }
	} else if(mtval) {
	    int errreg = mtval >> 5;
            void * errcap = errreg ? frame->registers[errreg - 1] : NULL;
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
//#endif
    /* Hack -- assume EBREAK comes from nlr_jump_fail() and unwind accordingly */
    if(mcause == 3) { printf("\x04\x04"); return ForceUnwind; }

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
    frame->pcc = __builtin_cheri_address_set(frame->pcc, (ptraddr_t)&nlr_jump);
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
    void ** stack = __builtin_cheri_stack_get();
    gc_collect_start();
    gc_collect_root(stack, ((size_t)MP_STATE_THREAD(stack_top) - (size_t)stack) / sizeof(mp_obj_t));
    gc_collect_end();
    //gc_dump_info(&mp_plat_print);
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

