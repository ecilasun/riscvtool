This project contains a set of helper utilities and sample code, as well as the bootloader ROM image for ecrv32 RISC-V SoC found here: https://github.com/ecilasun/NekoIchi

# Prerequisites

You'll need the risc-v toolchain from https://github.com/riscv/riscv-gnu-toolchain or its precompiled version (make sure to have support for 32bit ELF and rv32imc variant)
You'll also need a working Python so that the WAF build system can build the riscvtool.

# Building riscvtool

Before you can build the riscvtool itself, use the ctrl+shift+b shortcut in Visual Studio Code and select 'configure'. After this initial step you can use the same shortcut and select 'build'.
Alternatively, you can use the following command sequences:
```
# To configure (required only once:
python waf --out='build/release' configure

# To build after code changes:
python waf build -v

# To 'clean' the output binaries
python waf clean
```

# Building the ROM for NekoIchi

NekoIchi has a different architecture and currently doesn't have a UART loader. To build the ROM files, simply run:

```
./build.sh
```

and the ROM_nekoichi.coe file will be generated. You can then copy this over the BIOS.coe file in the source directory of NekoIchi.

You can find NekoIchi SoC here: https://github.com/ecilasun/NekoIchi

# Building the samples for NekoIchi

Currently there's no 'make' file to build the sample executables. Please use the following to build them:

```
./build.sh
```

at the root level of this project. This will generate the following samples and the ROM image (.COE) files for them in case you wish to paste them directly as ROM files into the memory initialization file of the Verilog source.

With the exception of the ROM image, you can use a reasonable amount of C++11 code at its basic level, and the uploader will take care of submitting each code / data section and takes care of branching to the entry point, so not much magic is required in the build.sh file.

```
ROM - the device ROM file, also generates a .COE (coefficient) file for Vivado, compiled using base instruction set only
miniterm - a small terminal program, connect to device serial port (usually at /dev/ttyUSB1) at 115200 baud, 1 stop bit, no xon/off and type help in the terminal window
instructiontest - test the compressed instructions and math instructions
sdcardtest - SD card FAT access demo, acts as memory mapped IO test
mandelbrot - draws a mandelbrot figure on the video buffer
rastertest - raster out flicker / tearing test using an offscreen draw target
```

As an example, to upload the miniterm.elf to the SoC, given that the USB cable is connected and your COM port is set up properly, use the following command line:
```
sudo ./build/release/riscvtool miniterm.elf -sendelf 0x00000000
```

If everything went well, you should be seeing an output similar to the following:
```
Program PADDR 0x00010000 relocated to 0x00000000
Executable entry point is at 0x00010336 (new relative entry point: 0x00000336)
Sending ELF binary over COM4 @115200 bps
sending '.text' @0x00000074 len:00001AAC off:00000074...done (0x00001AB8 bytes written)
sending '.rodata' @0x00001B20 len:0000197A off:00001B20...done (0x00001986 bytes written)
sending '.eh_frame' @0x0000449C len:00000458 off:0000349C...done (0x00000464 bytes written)
sending '.init_array' @0x000048F4 len:00000008 off:000038F4...done (0x00000014 bytes written)
sending '.fini_array' @0x000048FC len:00000004 off:000038FC...done (0x00000010 bytes written)
sending '.data' @0x00004900 len:00000448 off:00003900...done (0x00000454 bytes written)
sending '.got' @0x00004D48 len:00000038 off:00003D48...done (0x00000044 bytes written)
sending '.sdata' @0x00004D80 len:0000000C off:00003D80...done (0x00000018 bytes written)
sending '.bss' @0x00004D8C len:00004F88 off:00003D8C...done (0x00004F94 bytes written)
Branching to 0x00000336
```

NOTE: Please make sure that the default load address is 0x00010000 unless you really want to offset the binary for some reason. Take care to keep your binary away from address range 0x00000000-0x00002000 so that the loader code does not get overwritten while loading your binary.

NOTE: If the RISC-V compiler binaries (riscv64-unknown-elf-gcc or riscv64-unknown-elf-g++) are missing from your system, please follow the instructions at https://github.com/riscv/riscv-gnu-toolchain (especially useful is the section with multilib, try to build rv32i / rv32if / rv32imf / rv32imaf libraries, as there's currently no compressed instruction support on NekoIchi)
