# Setup

XLEN ?= 32

default: all

# Directories

src_dir = .
corelib_dir = ../SDK
memtest_libs = ../../3rdparty/memtest
nanojpeg_libs = ../../3rdparty/nanojpeg

# Executable folders

folders = \
	helloworld \
	adc \
	memtest \
	syscalltest \
	dmatest \
	tinyrtty \
	tinyrt \
	mandelbrot \
	jpegview \
	led \
	vgmplayer \
	usbc \
	dump \

# Rules

RISCV_PREFIX ?= riscv64-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)g++
RISCV_OBJDUMP ?= $(RISCV_PREFIX)objdump
RISCV_GCC_OPTS ?= -mcmodel=medany -std=c++14 --param l1-cache-line-size=64 --param l1-cache-size=16 -Wall -Ofast -march=rv32imc -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm

incs  += -I$(src_dir) -I$(corelib_dir) -I$(memtest_libs)/ -I$(nanojpeg_libs)/ $(addprefix -I$(src_dir)/, $(folders))
libs += $(wildcard $(corelib_dir)/*.S) $(wildcard $(corelib_dir)/*.c) $(wildcard $(memtest_libs)/*.c) $(wildcard $(nanojpeg_libs)/*.c)
objs  :=

define compile_template
$(1).elf: $(wildcard $(src_dir)/$(1)/*) $(wildcard $(src_dir)/*)
	$$(RISCV_GCC) $$(incs) -o $$@ $(wildcard $(src_dir)/$(1)/*.cpp) $$(libs) $$(RISCV_GCC_OPTS)
$(1).txt: $(wildcard $(src_dir)/$(1)/*) $(wildcard $(src_dir)/*)
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
