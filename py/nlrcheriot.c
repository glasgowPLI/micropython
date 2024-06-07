/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2017 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mpstate.h"

#if MICROPY_NLR_CHERIOT

__asm(
    ".type nlr_push, %function\n"
    "nlr_push:		\n"
    ".global nlr_push	\n"
    "csc cgp, 16(ca0)	\n"
    "csc cra, 24(ca0)	\n"
    "csc csp, 32(ca0)	\n"
    "csc cs0, 40(ca0)	\n"
    "csc cs1, 48(ca0)	\n"
    "j nlr_push_tail	\n"
    );
    

NORETURN void nlr_jump(void *val) {
    MP_STATIC_ASSERT(offsetof(nlr_buf_t, regs) == 16); // asm assumes it
    MP_NLR_JUMP_HEAD(val, top)

    __asm volatile (
        "cmove   ca0, %0             \n" // a0 points to nlr_buf
        "clc cgp, 16(ca0)          \n" // restore regs...
        "clc cra, 24(ca0)         \n"
        "clc csp, 32(ca0)         \n"
        "clc cs0, 40(ca0)         \n"
        "clc cs1, 48(ca0)         \n"
        "addi a0, zero, 1          \n" // non-local return
        "cret                      \n" // return
        :         
        : "C" (top)
        : "memory"      
        );

    MP_UNREACHABLE
}

#endif // MICROPY_NLR_CHERIOT
