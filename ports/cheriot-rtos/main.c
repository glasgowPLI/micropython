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

#if MICROPY_ENABLE_COMPILER
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#endif

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[16384]; //[MICROPY_HEAP_SIZE];
#endif

#include <stdio.h>

void __cheri_compartment("mp_main") mp_main(void) {
    MP_STATE_THREAD_HACK_INIT
    *MMIO_CAPABILITY(uint32_t, gpio) = 0xaa;
    printf("Test\n");
    int stack_dummy;
    stack_top = (char *)&stack_dummy;
    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    printf("GC initialised\n");
    #endif
    mp_init();
    printf("Micropython initialised\n");
    #if MICROPY_ENABLE_COMPILER
    #if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
    #else
    pyexec_friendly_repl();
    #endif
    // do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
    // do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);
    #else
#error "We don't want to be using frozen modules"
    pyexec_frozen_module("frozentest.py", false);
    #endif
    mp_deinit();
}

enum ErrorRecoveryBehaviour compartment_error_handler(struct ErrorState *frame, size_t mcause, size_t mtval) {
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
    while(1);//mp_raise_OSError(mcause);    
}


#if MICROPY_ENABLE_GC
void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
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
    while (1) {
        ;
    }
}

void NORETURN __fatal_error(const char *msg) {
    while (1) {
        ;
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif

