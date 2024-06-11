#define mp_hal_ticks_ms() (0UL)

#define mp_hal_set_interrupt_char(c) ((void)c)

#define MP_HAL_FUNC __cheri_compartment("mp_hal")

int MP_HAL_FUNC _mp_hal_stdin_rx_chr(void);
void MP_HAL_FUNC _mp_hal_stdout_tx_str(const char *str);
mp_uint_t MP_HAL_FUNC _mp_hal_stdout_tx_strn(const char *str, size_t len);
void MP_HAL_FUNC _mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);

#ifdef MP_VM_COMP
#define mp_hal_stdin_rx_chr() MP_STATE_THREAD_HACK_SPILL_FOR(_mp_hal_stdin_rx_chr(), int)
#define mp_hal_stdout_tx_str(str) MP_STATE_THREAD_HACK_SPILL_FOR_V(_mp_hal_stdout_tx_str(str))
#define mp_hal_stdout_tx_strn(str, len) MP_STATE_THREAD_HACK_SPILL_FOR(_mp_hal_stdout_tx_strn(str, len), mp_uint_t)
#define mp_hal_stdout_tx_strn_cooked(str, len) MP_STATE_THREAD_HACK_SPILL_FOR_V(_mp_hal_stdout_tx_strn_cooked(str, len))
#else
#define mp_hal_stdin_rx_chr _mp_hal_stdin_rx_chr
#define mp_hal_stdout_tx_str _mp_hal_stdout_tx_str
#define mp_hal_stdout_tx_strn _mp_hal_stdout_tx_strn
#define mp_hal_stdout_tx_strn_cooked _mp_hal_stdout_tx_strn_cooked
#endif

