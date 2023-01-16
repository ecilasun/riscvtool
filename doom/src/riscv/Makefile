CROSS ?= riscv64-unknown-elf-

CC = $(CROSS)gcc
OBJDUMP ?= $(CROSS)objdump
SIZE = $(CROSS)size

# use this for E32E
CFLAGS=-mcmodel=medany -std=gnu99 -Wall -O1 -flto -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc -lm -I.. -I../../../e32e/SDK/ -I../../../3rdparty/

# use this for FIVEPIPE
#CFLAGS=-mcmodel=medany -std=gnu99 -Wall -Ofast -flto -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc -lm -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/

# add -DFIVEPIPE to make this run on fivepipe, for e32e leave as-is
CFLAGS += \
	-DNORMALUNIX \
	$(NULL)


include ../sources.mk

# Filter out d_main, we provide our own simplified one
SOURCES_doom := $(filter-out d_main.c,$(SOURCES_doom))

# Filter out s_sound, we provide a dummy one
SOURCES_doom := $(filter-out s_sound.c,$(SOURCES_doom))

# Use fivepipe/SDK for FIVEPIPE, ../../../e32e/SDK/*.c  for E32E
SOURCES_doom_arch := \
	../../../e32e/SDK/*.c \
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
	$(NULL)

all: doom.elf doom.txt

doom.elf: $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(CC) $(CFLAGS) -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(SIZE) $@

clean:
	rm -f *.elf *.o *.gen.h doom.txt

%.txt: %.elf
	$(OBJDUMP) -x -D -S $< >> $@

.PHONY: all clean upload
