include ../../py/mkenv.mk

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION ?= 0

# include py core make definitions
include $(TOP)/py/py.mk
include $(TOP)/extmod/extmod.mk

CLLVM_DIR ?= ~/cheriot-llvm/
CRTOS_DIR ?= ../../../cheriot-rtos
UF2CONV ?= ~/.cargo/bin/uf2conv

CROSS_COMPILE = $(CLLVM_DIR)bin/
AS = $(CROSS_COMPILE)clang
CC = $(CROSS_COMPILE)clang
CXX = $(CROSS_COMPILE)clang++
LD = $(CROSS_COMPILE)ld.lld
SYS_INC = -I$(CRTOS_DIR)/sdk/include/c++-config -I$(CRTOS_DIR)/sdk/include/libc++ -I$(CRTOS_DIR)/sdk/include -I$(CRTOS_DIR)/sdk/include/platform/sunburst -I$(CRTOS_DIR)/sdk/include/platform/generic-riscv
#	  -I$(CRTOS_DIR)/sdk/include/platform/synopsis -I$(CRTOS_DIR)/sdk/include/platform/ibex 

INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)

INC += $(SYS_INC)
CFLAGS_CHERIOT_A7 =  -target riscv32-unknown-unknown -mcpu=cheriot -mabi=cheriot -mxcheri-rvc -mrelax -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables -fno-c++-static-destructors -fno-rtti 
CFLAGS += $(INC) -Wall -Werror -std=c2x -nostdlib $(CFLAGS_CHERIOT_A7) $(COPT) -DNO_ALLOCA_H -DCHERIOT_IBEX -DIBEX -DSUNBURST -DSUNBURST_SHADOW_BASE=0x30000000 -DSUNBURST_SHADOW_SIZE=0x4000 -DCPU_TIMER_HZ=33000000 -DTICK_RATE_HZ=100 -DCHERIOT_NO_AMBIENT_MALLOC -DDEVICE_EXISTS_shadow -DDEVICE_EXISTS_clint -DDEVICE_EXISTS_uart -DDEVICE_EXISTS_gpio -DDEVICE_EXISTS_gpio -DDEVICE_EXISTS_plic -DREVOKABLE_MEMORY_START=0x00100080
LDFLAGS += --script=cheriot-a7-100t.ld --relax

CSUPEROPT = -Os # save some code space

# Tune for Debugging or Optimization
CFLAGS += -g  # always include debug info in the ELF
ifeq ($(DEBUG), 1)
CFLAGS += -O0
else
CFLAGS += -Os -DNDEBUG
endif

# Flags for optional C++ source code
CXXFLAGS += $(filter-out -std=c99,$(CFLAGS)) -std=c++20

LIBS =

SRC_C = mp_entry.c \
	shared/readline/readline.c \
	shared/runtime/pyexec.c \
	$(BUILD)/_frozen_mpy.c \
	machine_pin.c \
	machine_spi.c \
	machine_i2c.c

SRC_CXX = hal/i2c.cpp hal/uart.cpp hal/timer.cpp

SRC_QSTR += shared/readline/readline.c shared/runtime/pyexec.c mp_entry.c machine_pin.c machine_spi.c machine_i2c.c

SRC_HAL_C = shared/runtime/stdout_helpers.c
SRC_HAL_CXX = uart_core.cpp
	
SRC_MAIN_C =
SRC_MAIN_CXX = main.cpp 

OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_CXX:.cpp=.o))
OBJ += $(PY_O)

OBJ_HAL += $(addprefix $(BUILD)/, $(SRC_HAL_C:.c=.o))
OBJ_HAL += $(addprefix $(BUILD)/, $(SRC_HAL_CXX:.cpp=.o))

OBJ_MAIN += $(addprefix $(BUILD)/, $(SRC_MAIN_C:.c=.o))
OBJ_MAIN += $(addprefix $(BUILD)/, $(SRC_MAIN_CXX:.cpp=.o))

$(OBJ): CFLAGS += -DMP_VM_COMP -cheri-compartment=mp_vm
$(OBJ_HAL): CFLAGS += -cheri-compartment=mp_hal
$(OBJ_MAIN): CFLAGS += -cheri-compartment=main
$(BUILD)/mp_entry.o: CFLAGS += -UCHERIOT_NO_AMBIENT_MALLOC

all: $(BUILD)/micropython.uf2

all_objects : $(OBJ) $(OBJ_HAL) $(OBJ_MAIN)
.PHONY: all all_objects

$(BUILD)/_frozen_mpy.c: $(TOP)/tests/frozen/frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=mpz $< > $@

$(BUILD)/xmake.lua: xmake.lua.top xmake.lua.bottom 
	cat xmake.lua.top > $@
	echo "compartment(\"mp_vm\")" >> $@
	for OFILE in $(patsubst $(BUILD)/%,%,$(OBJ)) ; do echo "    add_files(\"$$OFILE\", {rule = \"utils.merge.object\"})" >> $@ ; done
	echo "compartment(\"mp_hal\")" >> $@
	for OFILE in $(patsubst $(BUILD)/%,%,$(OBJ_HAL)) ; do echo "    add_files(\"$$OFILE\", {rule = \"utils.merge.object\"})" >> $@ ; done
	echo "compartment(\"main\")" >> $@
	for OFILE in $(patsubst $(BUILD)/%,%,$(OBJ_MAIN)) ; do echo "    add_files(\"$$OFILE\", {rule = \"utils.merge.object\"})" >> $@ ; done
	cat xmake.lua.bottom >> $@

$(BUILD)/build/cheriot/cheriot/release/micropython: all_objects $(BUILD)/xmake.lua
	cd $(BUILD) && xmake config --sdk=$(CLLVM_DIR) --board=sonata && xmake 

#$(BUILD)/cpu0_iram.vhx: $(BUILD)/build/cheriot/cheriot/release/micropython
#	$(CLLVM_DIR)/bin/llvm-objcopy -O binary $< - | hexdump -v -e '"%08X" "\n"' > $@
#	echo >> $@

$(BUILD)/micropython.bin: $(BUILD)/build/cheriot/cheriot/release/micropython
	$(CLLVM_DIR)/bin/llvm-objcopy -O binary $< $@

$(BUILD)/micropython.uf2: $(BUILD)/micropython.bin
	$(UF2CONV) $< --base 0x00101000 --output $@

include $(TOP)/py/mkrules.mk
