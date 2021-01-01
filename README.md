This project contains a set of helper utilities and sample code, as well as the bootloader ROM image for NekoNeo RISCV SoC.

To build the sample code and the ROM, simply type build.bat at the root level of this project. The ROM image will be converted to a COE format for Vivado, which then needs to be replaced with the one in the SoC project as the boot ROM image. The 'miniterm' sample will produce a miniterm.elf binary at the root of the project folder.

Before you can build the riscvtool itself, use the ctrl+shift+b shortcut in Visual Studio Code and select 'configure'. After this initial step you can use the same shortcut and select 'build'.

To upload the miniterm.elf to the SoC, use the following command line:
```
.\build\release\riscvtool.exe .\miniterm.elf -sendelf 0x000001D0
```

Please make sure that the load address is not lower than 0x1D0 to not overwrite the loader code. After that point on, the range of memory 0x0-0x1CF becomes available for code use as the boot loader is no longer in use.

For the miniterm sample, you can send raw binaries to be displayed on screen. For example, to send a 256x192 raw 8bpp image to copy to the VRAM, you could use the following command line:
```
.\build\release\riscvtool.exe C:\Users\ecila\OneDrive\Desktop\my8bitimage.raw -sendraw 0x80000000
```

NekoOne SoC specs:

```
32bit RISCV architecture (rv32im model)
16Kbytes ROM
64Kbytes of RAM
48Kbytes of VRAM driving 256x192 @8bpp single framebuffer
Memory mapped I/O
```

Memory layout:

```
ROM: 0x0000 - 0x1000 (4 Kbytes, DWORD addressible, read only, copied to RAM at hardware boot time)
USERRAM: 0x00000220 - 0x00014000 (80 Kbytes, BYTE addressible, read & write, 0x0-0x220 usable after bootloader loads program at 0x220)
VRAM: 0x80000000 - 0x8000C000 (48 Kbytes, 8bpp, BYTE addressible, write only)
IOSTATUS: 0x60000000 - ? (read only) [DWORD at +0: uart byte count]
IOREAD: 0x50000000 - ? (read only) [DWORD at +0: incoming uart byte]
IOWRITE: 0x40000000 - ? (write only) [DWORD at +0: byte to send]
```

External devices:

```
External VGA via VGA-pmod (output fully implemented)
SDCard via sdcard-pmod (not implemented yet)
GPIO support (not implemented yet)
```

Internal devices:

```
CPU at 100Mhz core clock
VGA scanout at 25Mhz core clock
UART @256000bauds with custom clock divider sourced from CPU clock
```