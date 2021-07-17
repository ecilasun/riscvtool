# Setup

XLEN ?= 32

default: all

# Directories

src_dir = .
corelib_dir = ../nekoSDK
thirdparty_libs = ../3rdparty

# Executable folders

folders = \
	ddr3test \

# Rules

RISCV_PREFIX ?= riscv64-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)g++
#RISCV_GCC_OPTS ?= -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC
# Currently NekoNi is at I phase
RISCV_GCC_OPTS ?= -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32i -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC

incs  += -I$(src_dir) -I$(corelib_dir) -I$(thirdparty_libs)/ $(addprefix -I$(src_dir)/, $(folders))
objs  :=

define compile_template
$(1).elf: $(wildcard $(src_dir)/$(1)/*) $(wildcard $(src_dir)/*)
	$$(RISCV_GCC) $$(incs) $$(RISCV_GCC_OPTS) -o $$@ $(wildcard $(src_dir)/$(1)/*.cpp) $(wildcard $(corelib_dir)/*.cpp) $(wildcard $(thirdparty_libs)/**/*.c)
endef

$(foreach folder,$(folders),$(eval $(call compile_template,$(folder))))

# Build

folders_riscv_bin  = $(addsuffix .elf,  $(folders))

executables: $(folders_riscv_bin)

junk += $(folders_riscv_bin)

# Default

all: executables

# Clean

clean:
	rm -rf $(objs) $(junk)
