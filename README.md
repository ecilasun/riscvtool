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
ROM: 0x0000 - 0x4000 (16 Kbytes, DWORD access, read only, copied to RAM at hardware boot time)
USERRAM: 0x000001D0 - 0x00003FFF (64 Kbytes, DWORD / WORD / BYTE access, read & write)
VRAM: 0x80000000 - 0x8000C000 (48 Kbytes, 8bpp, DWORD / WORD / BYTE access, write only)
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