#ifndef MP_ENTRY_HH
#define MP_ENTRY_HH

#include "mp_entry.h"

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
    template<typename T> static constexpr char sig_chr_ty = sig_chr(static_cast<T>(0));
    template<> static constexpr char sig_chr_ty<mp_callback_t*> = 'C';
    template<> static constexpr char sig_chr_ty<const mp_callback_t*> = 'C';
    template<typename T> static constexpr char sig_chr_exact = 0;
    template<> static constexpr char sig_chr_exact<int> = 'i';
    template<> static constexpr char sig_chr_exact<unsigned> = 'I'; 
    template<> static constexpr char sig_chr_exact<float> = 'f';
    template<> static constexpr char sig_chr_exact<double> = 'd';
    template<> static constexpr char sig_chr_exact<const char*> = 's';
    template<> static constexpr char sig_chr_exact<SObj> = 'O';
    template<> static constexpr char sig_chr_exact<mp_callback_t> = 'C';
    template<typename T> static constexpr char sig_chr_exact<T*> = 'P';
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
};

#endif

