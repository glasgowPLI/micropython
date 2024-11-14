#ifndef MATH_H
#define MATH_H

#define isnan(arg) ({ float tmp = arg; tmp != tmp; }) //TODO
#define isinf(arg) ({ float tmp = arg; ((*(uint32_t*)&tmp) & 0x7fffffff) == 0x7f800000; })
#define isfinite(arg) ({ float tmp = arg; ((*(uint32_t*)&tmp) & 0x7f800000) != 0x7f800000; }) // TODO
#define signbit(arg) ({ float tmp = arg; ((*(uint32_t*)&tmp) & 0x80000000); }) // TODO
#define INFINITY (1.0/0.0)
typedef float float_t;
#include <stdint.h>
#define __uint32_t uint32_t
#define __int32_t int32_t
#define __uint8_t uint8_t
#define __int8_t int8_t

float powf(float base, float exponent);
static inline float nanf(const char *arg) { return (0.0f/0.0f); }

float sqrtf(float);
float logf(float);
float log1pf(float);
float atan2f(float, float);
float expf(float);
float expm1f(float);
float ldexpf(float, int);
float frexpf(float, int*);
float cosf(float);
float sinf(float);
float tanf(float);
float asinf(float);
float acosf(float);
float atanf(float);
float coshf(float);
float sinhf(float);
float tanhf(float);

float fmodf(float, float);
float ceilf(float);
float floorf(float);
float truncf(float);
static inline float fabsf(float a) { uint32_t* hack = (uint32_t*)&a; *hack &= 0x7fffffff; return a; }
static inline float copysignf(float a, float b) {  uint32_t* hack = (uint32_t*)&a; *hack &= 0x7fffffff; *hack |= signbit(b); return a; }
float nearbyintf(float);
float modff(float, float*);
float scalbnf(float arg, int exp);

#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4

#endif // MATH_H
