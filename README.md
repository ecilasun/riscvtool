This project contains a set of helper utilities and sample code, as well as the bootloader ROM image for ecrv32 RISC-V SoC found here: https://github.com/ecilasun/ecrv32

To build the sample code and the ROM, simply type build.bat at the root level of this project. The ROM image will be converted to a COE format for Vivado, which then needs to be replaced with the one in the SoC project as the boot ROM image. The 'miniterm' sample will produce a miniterm.elf binary at the root of the project folder.

Before you can build the riscvtool itself, use the ctrl+shift+b shortcut in Visual Studio Code and select 'configure'. After this initial step you can use the same shortcut and select 'build'.

To upload the miniterm.elf to the SoC, use the following command line:
```
./build/release/riscvtool miniterm.elf -sendelf 0x000001D0
```

Please make sure that the load address is not lower than 0x1D0 to not overwrite the loader code. After that point on, the range of memory 0x0-0x1CF becomes available for code use as the boot loader is no longer in use.

For the miniterm sample, you can send raw binaries to be displayed on screen. For example, to send a 256x192 raw 8bpp image to copy to the VRAM, you could use the following command line:
```
./build/release/riscvtool my8bitimage.raw -sendraw 0x80000000
```
