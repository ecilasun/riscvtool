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

# Building the ROM and samples for NekoIchi

To build the ROM file and the samples, either switch to the 'ROMs' or 'samples' directory and run:

```
make
```

You can also do the same with 'doom/src/riscv' and 'dhrystone' directories to build those programs.

This will create ROMnekoichi.coe file which is the ROM image for NekoIchi, alognside with some samples that riscvtool can upload. You can copy the ROM_nekoichi.COE files contents over the BIOS.coe file in the source directory of NekoIchi HDL code, which you can find at https://github.com/ecilasun/NekoIchi

With the exception of the ROM image, you can use a reasonable amount of C++11 code at some level, and the uploader will take care of submitting each code / data section and takes care of branching to the entry point, so not much magic is required in the build.sh file.

Here is a list of the binary outputs after running the build command:

```
ROM_nekoichi - the device ROM file, also generates a .COE (coefficient) file for Vivado
miniterm - a small terminal program, connect to device serial port (usually at /dev/ttyUSB1) at 115200 baud, 1 stop bit, no xon/off and type help in the terminal window
mandelbrot - draws a mandelbrot figure on the video buffer
gpupipetest - tests the GPU rasterizer / synchronization mechanisms and the realtime clock
```

# Uploading binaries

As an example, to upload the miniterm.elf to the SoC, given that the USB cable is connected and your COM port is set up properly, use a command line similar to the following (while changing the name of your USB device to match)
```
./build/release/riscvtool samples/miniterm.elf -sendelf 0x10000 /dev/ttyUSB1
```

P.S. Always use 0x00010000 as base address if you haven't used a custom linker script.

P.P.S. If command line complains about accessing the COM port you may need to run the risctool using sudo.

If everything went well, you should see an output similar to the following:
```
Program PADDR 0x00010000 relocated to 0x00010000
Executable entry point is at 0x00010528 (new relative entry point: 0x00010528)
Sending ELF binary over COM4 @115200 bps
SEND: '.text' @0x00010074 len:0000311C off:00000074...done (0x0000311A+0xC bytes written)
SEND: '.rodata' @0x00013190 len:00001AEC off:00003190...done (0x00001AEA+0xC bytes written)
SEND: '.eh_frame' @0x00015000 len:000004DC off:00005000...done (0x000004DA+0xC bytes written)
SEND: '.init_array' @0x000154DC len:00000008 off:000054DC...done (0x00000006+0xC bytes written)
SEND: '.fini_array' @0x000154E4 len:00000004 off:000054E4...done (0x00000002+0xC bytes written)
SEND: '.data' @0x000154E8 len:00000440 off:000054E8...done (0x0000043E+0xC bytes written)
SEND: '.got' @0x00015928 len:0000002C off:00005928...done (0x0000002A+0xC bytes written)
SEND: '.sdata' @0x00015954 len:00000008 off:00005954...done (0x00000006+0xC bytes written)
SKIP: '.bss' @0x0001595C len:00004F8C off:0000595C
Branching to 0x00010528
```

NOTE: Take care to keep your binary away from address range 0x20000000-0x2001FFFF so that the loader code does not get overwritten while loading your binary. All addresses from 0x00000000 up to 0x0FFFFFFF are available for program use otherwise (an 256Mbyte address range)

# Debugging with GDB

If you've linked your program with the debug.cpp file and have necessary trap handlers installed (as in the romexplerimental sample), you can then attach with GDB to debug your code:

```
riscv64-unknown-elf-gdb -b 115200 --tui samples/romexperimental.elf
target remote /dev/ttyUSB1
```

This will break into the currently executing program. Use 'c' command to resume execution, or Ctrl+C to break at an arbitrary breakpoint. You can also set breakpoints when the program is paused by using 'b DrawConsole' for instance. On resume with 'c' the program will be stopped at the new breakpoint address.

# RISC-V compiler tools

NOTE: If the RISC-V compiler binaries (riscv64-unknown-elf-gcc or riscv64-unknown-elf-g++) are missing from your system, please follow the instructions at https://github.com/riscv/riscv-gnu-toolchain
It is advised to build the rv32i / rv32if / rv32imf / rv32imaf libraries

There's currently no compressed instruction support on NekoIchi, and there's no plan to do it in the near future.
