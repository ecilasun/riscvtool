CROSS ?= riscv64-unknown-elf-

CC = $(CROSS)gcc
OBJDUMP ?= $(CROSS)objdump
SIZE = $(CROSS)size

# use this for E32E
CFLAGS=-mcmodel=medany -std=gnu99 -Wall -O1 -flto -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc -lm -I.. -I../../../e32e/SDK/ -I../../../3rdparty/

# use this for FIVEPIPE
#CFLAGS=-Wall -O2 -march=rv32im -mabi=ilp32 -ffreestanding -flto -nostartfiles -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/

#CFLAGS=-Wall -lgcc -lm -Ofast -march=rv32im -mabi=ilp32 -static -ffreestanding -fomit-frame-pointer -Wl,--gc-section --specs=nano.specs -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/
#CFLAGS=-mcmodel=medany -std=gnu99 -fPIC -static -ffreestanding -Wall -O2 -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc -lm -I../../../fivepipe/SDK/ -I../../../3rdparty/
#CFLAGS=-mcmodel=medany -std=gnu99 -fPIC -static -ffreestanding -fomit-frame-pointer -Wall -O2 -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections,-strip-debug -specs=nosys.specs -lm -I.. -I../../../fivepipe/SDK/ -I../../../3rdparty/

# add -DFIVEPIPE to make this run on fivepipe, for e32e leave as-is
CFLAGS += \
	-DNORMALUNIX \
	$(NULL)


include ../sources.mk

# Filter out d_main, we provide our own simplified one
SOURCES_doom := $(filter-out d_main.c,$(SOURCES_doom))

# Filter out s_sound, we provide a dummy one
SOURCES_doom := $(filter-out s_sound.c,$(SOURCES_doom))

# add start.S for FIVEPIPE
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

# use $(CC) $(CFLAGS) -Wl,-Bstatic,-T,riscv.lds,--strip-debug -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch) for FIVEPIPE

doom.elf: $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(CC) $(CFLAGS) -o $@ $(addprefix ../,$(SOURCES_doom)) $(SOURCES_doom_arch)
	$(SIZE) $@

clean:
	rm -f *.bin *.hex *.elf *.o *.gen.h doom.txt

%.txt: %.elf
	$(OBJDUMP) -x -D -S $< >> $@

.PHONY: all clean upload
