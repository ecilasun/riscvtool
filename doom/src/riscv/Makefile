CROSS ?= riscv64-unknown-elf-

CC = $(CROSS)gcc
OBJDUMP ?= $(CROSS)objdump
SIZE = $(CROSS)size

# -ffunction-sections -fdata-sections -Wl,-gc-sections,-strip-debug 
# --specs=nano.specs -std=c++14 -> need to use g++
#CFLAGS=-mcmodel=medany -Wall -O2 -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc -lm -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/
#CFLAGS=-Wall -lgcc -lm -Ofast -march=rv32im -mabi=ilp32 -static -ffreestanding -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/
#CFLAGS=-mcmodel=medany -std=gnu99 -fPIC -static -ffreestanding -Wall -O2 -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc -lm -I../../../fivepipe/SDK/ -I../../../3rdparty/
#CFLAGS=-mcmodel=medany -std=gnu99 -fPIC -static -ffreestanding -fomit-frame-pointer -Wall -O2 -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections,-strip-debug -specs=nosys.specs -lm -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/
CFLAGS=-Wall -O2 -march=rv32im -mabi=ilp32 -ffreestanding -flto -nostartfiles -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/

CFLAGS += \
	-DNORMALUNIX \
	$(NULL)


include ../sources.mk

# Filter out d_main, we provide our own simplified one
SOURCES_doom := $(filter-out d_main.c,$(SOURCES_doom))

# Filter out s_sound, we provide a dummy one
SOURCES_doom := $(filter-out s_sound.c,$(SOURCES_doom))

SOURCES_doom_arch := \
	../../../fivepipe/SDK/*.c \
	../../../3rdparty/fat32/*.c \
	d_main.c \
	i_main.c \
	i_net.c \
	i_sound.c \
	i_system.c \
	i_video.c \
	s_sound.c \
	console.c  \
	mini-printf.c \
	start.S \
	$(NULL)

all: doom.elf doom.txt

doom.elf: $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(CC) $(CFLAGS) -Wl,-Bstatic,-T,riscv.lds,--strip-debug -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(SIZE) $@

clean:
	rm -f *.bin *.hex *.elf *.o *.gen.h doom.txt

%.txt: %.elf
	$(OBJDUMP) -x -D -S $< >> $@

.PHONY: all clean upload
