#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <compartment.h>

#include "mp_entry.h"
#include "mphalport.h"

#include <stdio.h>

void __cheri_compartment("main") entry(void) {
    *MMIO_CAPABILITY(uint32_t, gpio) = 0xaa;
    printf("Test\n");
    mp_vminit();
    while(1) switch(mp_hal_stdin_rx_chr()) {
        #if MICROPY_ENABLE_COMPILER
	case 'r':
            if(!mp_friendly_repl()) continue;
	    printf("REPL exited with a failure\n");
	    return;
        #endif
	case 's':
	    if(!mp_exec_str_single("print('foo', 7*8*9, [ 'bar', 'baz' ])")) continue;
	    printf("String execution exited with a failure\n");
	case 'f':
	    if(!mp_exec_frozen_module("frozentest.py")) continue;
	    printf("Frozen module exited with a failure\n");
	    return;
	    break;
        case 'q':
	    mp_vmexit();
	    printf("VM Exited");
	    return;
    }
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
    while(1);    
}


