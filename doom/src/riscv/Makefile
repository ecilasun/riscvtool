CROSS ?= riscv64-unknown-elf-

CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
SIZE = $(CROSS)size
ICEPROG = iceprog

#-ffunction-sections -fdata-sections -Wl,-gc-sections,--strip-debug 

CFLAGS=-mcmodel=medany -fPIC -lgcc -lm -Wall -O2 -march=rv32im -mabi=ilp32 -ffreestanding -flto -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -I..

CFLAGS += \
	-DNORMALUNIX \
	$(NULL)


include ../sources.mk

# Filter out d_main, we provide our own simplified one
SOURCES_doom := $(filter-out d_main.c,$(SOURCES_doom))

# Filter out s_sound, we provide a dummy one
SOURCES_doom := $(filter-out s_sound.c,$(SOURCES_doom))


SOURCES_doom_arch := \
	d_main.c \
	i_main.c \
	i_net.c \
	i_sound.c \
	i_system.c \
	i_video.c \
	s_sound.c \
	console.c  \
	libc_backend.c  \
	mini-printf.c \
	config.c \
	ff.c \
	diskio.c \
	gpu.c \
	sdcard.c \
	$(NULL)

# start.S

all: doom-riscv.elf

doom-riscv.elf: $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(CC) $(CFLAGS) -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(SIZE) $@

clean:
	rm -f *.bin *.hex *.elf *.o *.gen.h


%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

prog: doom-riscv.bin
	$(ICEPROG) -o 1M $<

prog_wad: data/doomu.wad
	$(ICEPROG) -o 2M $<


.PHONY: all clean prog prog_wad
