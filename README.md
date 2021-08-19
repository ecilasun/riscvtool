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

# Building the ROM and samples for Neko series

To build the ROM file and the samples, either switch to the 'ROMs' or 'samples' directory under nekoichi or nekosan depending on the device you are running, and run:

```
make
```

You can also do the same with 'doom/src/riscv' and 'dhrystone' directories to build those programs.

This will create ROMneko*.coe file which is the ROM image for Neko*, alongside some samples that riscvtool can upload to test the device. You can copy the ROMneko*.COE files contents over the BIOS.COE file in the source directory of Neko* HDL code, which you can find at either of https://github.com/ecilasun/NekoIchi or https://github.com/ecilasun/NekoSan

With the exception of the ROM image, you can use a fair amount of C++14 code at some level, and the uploader will take care of submitting each code / data section and takes care of branching to the entry point, so not much magic is required in the build.sh file. This means in normal situations you should not require a custom linker script to run ELF files.

# Uploading binaries

As an example, to upload the miniterm.elf to the NekoIchi variant, given that the USB cable is connected and your COM port is set up properly, use a command line similar to the following (while changing the name of your USB device to match)
```
./build/release/riscvtool nekoichi/samples/miniterm.elf -sendelf 0x10000 /dev/ttyUSB1
```

P.S. Always use 0x00010000 as base address as this is the default, unless you somehow change it.

P.P.S. If command line complains about accessing the COM port you may need to run the risctool using sudo.

If everything went well, you should see an output similar to the following:
```
Program PADDR 0x00010000 relocated to 0x00010000
Executable entry point is at 0x00010528 (new relative entry point: 0x00010528)
Sending ELF binary over COM4 @115200 bps
SENDING: '.text' @0x00010074 len:0000311C off:00000074...done (0x0000311A+0xC bytes written)
SENDING: '.rodata' @0x00013190 len:00001AEC off:00003190...done (0x00001AEA+0xC bytes written)
SENDING: '.eh_frame' @0x00015000 len:000004DC off:00005000...done (0x000004DA+0xC bytes written)
SENDING: '.init_array' @0x000154DC len:00000008 off:000054DC...done (0x00000006+0xC bytes written)
SENDING: '.fini_array' @0x000154E4 len:00000004 off:000054E4...done (0x00000002+0xC bytes written)
SENDING: '.data' @0x000154E8 len:00000440 off:000054E8...done (0x0000043E+0xC bytes written)
SENDING: '.got' @0x00015928 len:0000002C off:00005928...done (0x0000002A+0xC bytes written)
SENDING: '.sdata' @0x00015954 len:00000008 off:00005954...done (0x00000006+0xC bytes written)
SKIPPING: '.bss' @0x0001595C len:00004F8C off:0000595C
Branching to 0x00010528
```

NOTE: Take care to keep your binary away from address range 0x20000000-0x2FFFFFFF so that the loader code does not get overwritten while loading your binary. All addresses from 0x00000000 up to 0x0FFFFFFF are available for program use otherwise (an 256Mbyte address range), and 0x20000000-0x2FFFFFFF space can be accessed after the program starts running, to be utilized as uncached fast RAM.

# Debugging with GDB

If you've linked your program with the debug.cpp file and have necessary trap handlers installed (as in the romexplerimental sample), you can then attach with GDB to debug your code:

```
riscv64-unknown-elf-gdb -b 115200 --tui nekoichi/samples/romexperimental.elf
target remote /dev/ttyUSB1
```

This will break into the currently executing program. Use 'c' command to resume execution, or Ctrl+C to break at an arbitrary breakpoint. You can also set breakpoints when the program is paused by using 'b DrawConsole' for instance. On resume with 'c' the program will be stopped at the new breakpoint address.

Please note that this is an entirely software based feature and its usage pattern / implementation may change over time.

# RISC-V compiler tools

NOTE: If the RISC-V compiler binaries (riscv64-unknown-elf-gcc or riscv64-unknown-elf-g++) are missing from your system, please follow the instructions at https://github.com/riscv/riscv-gnu-toolchain
It is advised to build the rv32i / rv32if / rv32imf / rv32imaf libraries

There's currently no compressed instruction support on any Neko SoCs, and there's no plan to do it in the foreseeable future. If this changes, code and documentation will be updated to reflect the changes.
