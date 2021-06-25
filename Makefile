XLEN ?= 32

default: all

src_dir = samples
corelib_dir = nekolib
thirdparty_libs = 3rdparty

#--------------------------------------------------------------------
# Sources
#--------------------------------------------------------------------

bmarks = \
	mandelbrot \
	modplayer \
	ddr3test \
	romexperimental \
	miniterm \
	gputest \
	audiotest \

#--------------------------------------------------------------------
# Build rules
#--------------------------------------------------------------------

RISCV_PREFIX ?= riscv64-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)g++
RISCV_GCC_OPTS ?= -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC
# RISCV_LINK ?= $(RISCV_GCC) -T $(src_dir)/common/test.ld $(incs)
# RISCV_LINK_OPTS ?= -static -nostdlib -nostartfiles -lm -lgcc -T $(src_dir)/common/test.ld
#RISCV_OBJDUMP ?= $(RISCV_PREFIX)objdump --disassemble-all --disassemble-zeroes --section=.text --section=.text.startup --section=.text.init --section=.data
#RISCV_SIM ?= spike --isa=rv$(XLEN)gc

incs  += -I$(src_dir) -I$(corelib_dir) -I$(thirdparty_libs)/ $(addprefix -I$(src_dir)/, $(bmarks))
objs  :=

define compile_template
$(1).elf: $(wildcard $(src_dir)/$(1)/*) $(wildcard $(src_dir)/*)
	$$(RISCV_GCC) $$(incs) $$(RISCV_GCC_OPTS) -o $$@ $(wildcard $(src_dir)/$(1)/*.cpp) $(wildcard $(corelib_dir)/*.cpp) $(wildcard $(thirdparty_libs)/**/*.c)
endef

# $$(RISCV_LINK_OPTS)

$(foreach bmark,$(bmarks),$(eval $(call compile_template,$(bmark))))

#------------------------------------------------------------
# Build and run benchmarks on riscv simulator

bmarks_riscv_bin  = $(addsuffix .elf,  $(bmarks))
#bmarks_riscv_dump = $(addsuffix .elf.dump, $(bmarks))
#bmarks_riscv_out  = $(addsuffix .elf.out,  $(bmarks))

#$(bmarks_riscv_dump): %.elf.dump: %.elf
#	$(RISCV_OBJDUMP) $< > $@

#$(bmarks_riscv_out): %.elf.out: %.elf
#	$(RISCV_SIM) $< > $@

executables: $(bmarks_riscv_bin)
# run: $(bmarks_riscv_out)

junk += $(bmarks_riscv_bin)
# $(bmarks_riscv_dump) $(bmarks_riscv_hex) $(bmarks_riscv_out)

#------------------------------------------------------------
# Default

all: executables

#------------------------------------------------------------
# Clean up

clean:
	rm -rf $(objs) $(junk)
