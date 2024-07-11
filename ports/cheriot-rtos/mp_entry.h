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
extern int __cheri_compartment("mp_vm") mp_exec_func_v(SObj ctx, const char * func, void * ret, int n_args, const char * sig, va_list ap);
static inline int mp_exec_func(SObj ctx, const char * func, void * ret, int n_args, const char * sig, ...) {
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
    void * p;
    const void * q;
} mp_cb_arg_t;

typedef struct {
    mp_cb_arg_t __cheri_callback (*func)(void * data, mp_cb_arg_t * args);
    void * data;
    int n_args;
    char * sig;
} mp_callback_t;

#ifdef __cplusplus
class MicropythonContext {
  private:
    SObj ctx;
    constexpr MicropythonContext(SObj _ctx) : ctx(_ctx) {}
    /* Helper functions for inferring the signature string for mp_exec_func()
     *
     * Arguments are inferred using sig_chr_ty<T> which in turn uses the sig_chr(x)
     * overloads to capture the argument type after promotions; return types are
     * inferred using sig_chr_exact<T> to capture the exact type.
     */
    static constexpr char sig_chr(int i)          { return 'i'; }
    static constexpr char sig_chr(unsigned i)     { return 'I'; }
    static constexpr char sig_chr(float f)        { return 'f'; }
    static constexpr char sig_chr(double d)       { return 'd'; }
    static constexpr char sig_chr(const char * s) { return 's'; }
    static constexpr char sig_chr(SObj o)         { return 'O'; }
    static constexpr char sig_chr(void * p)       { return 'P'; }
    static constexpr char sig_chr(mp_callback_t p){ return 'C'; }
    template<typename T> static constexpr char sig_chr_ty = sig_chr(static_cast<T>(0));
    template<typename T> static constexpr char sig_chr_exact = 0;
    template<> static constexpr char sig_chr_exact<int> = 'i';
    template<> static constexpr char sig_chr_exact<unsigned> = 'I'; 
    template<> static constexpr char sig_chr_exact<float> = 'f';
    template<> static constexpr char sig_chr_exact<double> = 'd';
    template<> static constexpr char sig_chr_exact<const char*> = 's';
    template<> static constexpr char sig_chr_exact<SObj> = 'O';
    template<> static constexpr char sig_chr_exact<mp_callback_t> = 'C';
    template<typename T> static constexpr char sig_chr_exact<T*> = 'P';
    template<typename R, typename... Ts>
    static inline mp_cb_arg_t cb_wrapper_aux(std::function<R(Ts...)> * func, mp_cb_arg_t * empty, Ts... cargs) {
        return (*func)(cargs...);
    }
    template<typename R, typename... Ts, typename T, typename... Us>
    static inline mp_cb_arg_t cb_wrapper_aux(std::function<R(Ts..., T, Us...)> * func, mp_cb_arg_t * pyargs, Ts... cargs) {
        return cb_wrapper_aux(func, pyargs + 1, cargs..., argconv<T>(pyargs[0]));
    }
    template<typename R, typename... Ts> static mp_cb_arg_t __cheri_callback cb_wrapper(std::function<R(Ts...)> * func, mp_cb_arg_t * args) {
        return cb_wrapper_aux<R, Ts...>(func, args);
    }
  public:
    [[nodiscard]] MicropythonContext(MicropythonContext&& src) : ctx(src.ctx) { src.ctx = INVALID_SOBJ; }
    [[nodiscard]] static std::optional<MicropythonContext> create(size_t heapsize) {
        SObj ctx = mp_vminit(heapsize);
	return (__builtin_cheri_tag_get(ctx)) ? std::optional(MicropythonContext(ctx)) : std::nullopt;
    } 
    int restart() {
	return mp_vmrestart(ctx);    
    }
    ~MicropythonContext() { if(ctx) mp_vmexit(ctx); }
    int exec_str_single(const char * src) { return mp_exec_str_single(ctx, src); }
    int exec_str_file(const char * src) { return mp_exec_str_file(ctx, src); }
    int friendly_repl() { return mp_friendly_repl(ctx); }
    int raw_repl() { return mp_raw_repl(ctx); }
    int var_repl() { return mp_var_repl(ctx); }
    int exec_frozen_module(const char * name) { return mp_exec_frozen_module(ctx, name); }
    template<typename R, typename... Ts>
    std::optional<R> exec_func(const char * func, Ts... args) {
        R ret;
	constexpr const char sig[sizeof...(Ts) + 1] = { sig_chr_exact<R>, sig_chr_ty<Ts>... };
	int err = mp_exec_func(ctx, func, &ret, sizeof...(Ts), sig, args...);
        return err ? std::nullopt : std::optional(ret);
    }
    template<std::same_as<void> V, typename... Ts>
    bool exec_func(const char * func, Ts... args) {
        constexpr const char sig[sizeof...(Ts) + 1] = { 'v', sig_chr_ty<Ts>... };
        int err = mp_exec_func(ctx, func, NULL, sizeof...(Ts), sig, args...);
        return !err;
    }
    int free_obj_handle(SObj obj) { return mp_free_obj_handle(ctx, obj); }
    template<typename R, typename... Ts> [[nodiscard]] static mp_callback_t leaf_callback(std::function<R(Ts...)> &func) {
	constexpr const char sig[sizeof...(Ts) + 1] = { sig_chr_exact<R>, sig_chr_exact<Ts>... };
        return { .func = &cb_wrapper<R, Ts...> , .data = &func , .n_args = sizeof...(Ts), .sig = sig };
    }
};

#endif

#endif

