# Setup

XLEN ?= 32

default: all

# Directories

src_dir = .

# Executable folders

folders = \
	syscalltest \

# Rules

RISCV_PREFIX ?= riscv64-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)g++
RISCV_OBJDUMP ?= $(RISCV_PREFIX)objdump
RISCV_GCC_OPTS ?= -std=c++14 -Wall -Ofast -march=rv32im -mabi=ilp32 -fPIC -lgcc -lm

incs  += -I$(src_dir)  $(addprefix -I$(src_dir)/, $(folders))
libs :=
objs  :=

define compile_template
$(1).elf: $(wildcard $(src_dir)/*) $(wildcard $(src_dir)/*)
	$$(RISCV_GCC) $$(incs) -o $$@ $(wildcard $(src_dir)/*.cpp) $$(libs) $$(RISCV_GCC_OPTS)
$(1).txt: $(wildcard $(src_dir)/*) $(wildcard $(src_dir)/*)
	$$(RISCV_OBJDUMP) $(src_dir)/$(1).elf -x -D -S >>  $(src_dir)/$(1).txt
endef

$(foreach folder,$(folders),$(eval $(call compile_template,$(folder))))

# Build

folders_riscv_bin  = $(addsuffix .elf,  $(folders))
folders_riscv_disasm = $(addsuffix .txt, $(folders))

executables: $(folders_riscv_bin)
disassembly: $(folders_riscv_disasm)

junk += $(folders_riscv_bin) $(folders_riscv_disasm)

# Default

all: executables disassembly

# Clean

clean:
	rm -rf $(objs) $(junk)
