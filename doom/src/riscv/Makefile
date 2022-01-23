CROSS ?= riscv64-unknown-elf-

CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
OBJDUMP ?= $(CROSS)objdump
SIZE = $(CROSS)size
UPLOADER = ../../../build/release/riscvtool

# -ffunction-sections -fdata-sections -Wl,-gc-sections,-strip-debug 
# --specs=nano.specs -std=c++14 -> need to use g++
CFLAGS=-mcmodel=medany -fPIC -lgcc -lm -Wall -O2 -march=rv32im -mabi=ilp32 -ffreestanding -flto -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -Wl,-strip-debug -I.. -I../../../3rdparty/ -I../../../e32/SDK/

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
	../../../3rdparty/fat32/ff.c \
	../../../3rdparty/fat32/diskio.c \
	../../../3rdparty/fat32/ffsystem.c \
	../../../3rdparty/fat32/ffunicode.c \
	../../../e32/SDK/core.c \
	../../../e32/SDK/sdcard.c \
	../../../e32/SDK/uart.c \
	../../../e32/SDK/spi.c \
	../../../e32/SDK/gpu.c \
	../../../e32/SDK/buttons.c \
	../../../e32/SDK/ringbuffer.c \
	$(NULL)

# start.S

all: doom-riscv.elf doom-riscv.txt

doom-riscv.elf: $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(CC) $(CFLAGS) -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(SIZE) $@

clean:
	rm -f *.bin *.hex *.elf *.o *.gen.h doom-riscv.txt

%.txt: %.elf
	$(OBJDUMP) -x -D -S $< >> $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

upload: doom-riscv.elf
	$(UPLOADER) $< -sendelf 0x10000 /dev/ttyUSB1

.PHONY: all clean upload
