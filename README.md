This project contains a set of helper utilities and sample code, as well as the bootloader ROM images for RISC-V SoC design variations found here: https://github.com/ecilasun

# Platforms supported

Currently ROM images and samples are provided for the following platform, under their respective directies:
- E32E (Most up-to-date and latest version actively developed)
- NekoIchi (ROM for the article over at http://displayoutput.com/)

# Prerequisites

You'll need the risc-v toolchain from https://github.com/riscv/riscv-gnu-toolchain or its precompiled version (make sure to have support for 32bit ELF and rv32imc variant)
You'll also need a working Python so that the WAF build system can build the riscvtool.

There's a convenience script in this directory that will automate this task for you. Simply run:

```
./buildrisctoolchain.sh
```

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

To build the ROM file and the samples, either switch to the 'ROMs' or 'samples' directory under any neko or E32 project depending on the device you are running, and use this on the command line:

```
make
```

You can also do the same with 'doom/src/riscv' and 'dhrystone' directories to build those programs. Please note that DOOM will always be locked to run on the latest design, which is currently E32E.

This will create a ROM*.coe file which is the ROM image for the specific platform you chose. You can copy the ROM*.COE files contents over the BIOS.COE file in the source directory of the HDL code. You can find the hardware design files at https://github.com/ecilasun/

With the exception of the ROM image, you can use a fair amount of C++14 code, and the uploader will take care of submitting each code / data section and takes care of branching to the entry point, so not much magic is required in the build.sh file. This means in normal situations you should not require a custom linker script to run ELF files, except the need to link to the core files in the SDK directory.

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

# Using SDCard instead

For some of the platforms, for example E32E, ELF files can live on an SDCard and be loaded from either a command line or with an onscreen menu. Simply copy across the sample files onto a FAT32 formatted SDCard and plug this into either a PMOD as shown on the device design or onto the onboard SDCard reader. For example, on E32E, one can use dir command to list the executables on disk and type in the name of the ELF file (without the .ELF extension) to run said executable.

# Debugging with GDB

If you've linked your program with the debug.cpp file (on some platforms) and have necessary trap handlers installed (as in the romexplerimental sample on some platforms), you can then attach with GDB to debug your code:

```
riscv64-unknown-elf-gdb -b 115200 --tui nekoichi/samples/romexperimental.elf
target remote /dev/ttyUSB1
```

This will break into the currently executing program. Use 'c' command to resume execution, or Ctrl+C to break at an arbitrary breakpoint. You can also set breakpoints when the program is paused by using 'b DrawConsole' for instance. On resume with 'c' the program will be stopped at the new breakpoint address.
 
Please note that this is an entirely software based feature and its usage pattern / implementation may change over time.

One very useful visual too to aid in debugging is gdbgui which you can find here:
https://www.gdbgui.com/gettingstarted/

# RISC-V compiler tools

NOTE: If the RISC-V compiler binaries (riscv64-unknown-elf-gcc or riscv64-unknown-elf-g++) are missing from your system, please follow the instructions at https://github.com/riscv/riscv-gnu-toolchain
It is advised to build the rv32i / rv32if / rv32imf / rv32imaf libraries

There's currently no compressed instruction support on any Neko SoCs, and there's no plan to do it in the foreseeable future. If this changes, code and documentation will be updated to reflect the changes.

If you want to work on Windows and don't want to compile the toolchain, you could use the following link and download the latest riscv-v-gcc installer executable (risc-v-gcc10.1.0.exe at the time of writing this)

https://gnutoolchains.com/risc-v/

This will place all the toolchain files under C:\SysGCC\risc-v by default and make sure to have that path added to your %PATH% by using the 'Add binary directory to %PATH%' option.

NOTE: One thing worth mentioning is that at this time that compiler toolchain was not aware of rv32imc_zicsr_zifencei so please set the makefile to use march=rv32imc in sample projects and ROM projects instead.

# NOTES

This tool is a simple environment with most things set up for all the platforms under https://github.com/ecilasun/ repo. It's mostly used internally, but if you take a design from that repo, this provides a good starting point to roll your own software. Be advised that all software has been tested with and geared to use a gcc-riscv 32bit environment, therefore any compiler changes and/or bit width changes are untested at this moment.

Adding tinysys as a test serial device via generic driver:
```
unplug the usb device
type following in a terminal
 echo FFFF 0001 >/sys/bus/usb-serial/drivers/generic/new_id
(if the echo fails you can use sudo nano and write FFFF 0001 to the new_id file and save/close it)
re-plug the usb device and start usbc on the device
now it should enumerate as a generic serial device with a warning in dmesg
```

