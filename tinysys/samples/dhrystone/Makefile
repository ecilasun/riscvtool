# Setup

default: all

# Directories

src_dir = .
corelib_dir = ../../SDK
#thirdparty_libs = ../../../3rdparty

# Rules

RISCV_PREFIX ?= riscv64-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)gcc
#RISCV_GCC_OPTS ?= -mcmodel=medany -std=gnu99 -Wall -Ofast -march=rv32imc -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm
RISCV_GCC_OPTS ?= -DPREALLOCATE=1 -mcmodel=medany -static -march=rv32imc -mabi=ilp32 -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-loop-distribute-patterns -nostdlib -nostartfiles -lm -lgcc

incs  += -I$(src_dir) -I$(corelib_dir)/ -I$(thirdparty_libs)/ -I$(src_dir)/.
libs += $(wildcard $(corelib_dir)/*.c) #$(wildcard $(thirdparty_libs)/**/*.c)
objs  := dyrystone.elf

$(foreach folder,$(folders),$(eval $(call compile_template,$(folder))))

# Build

dyrystone.elf: $(wildcard $(src_dir)/*)
	$(RISCV_GCC) $(incs) -o $@ $(wildcard $(src_dir)/*.c) $(wildcard $(src_dir)/*.S) $(libs) $(RISCV_GCC_OPTS) -T $(src_dir)/test.ld

junk += $(folders_riscv_bin)

# Default

all: dyrystone.elf

# Clean

clean:
	rm -rf $(objs) $(junk)
