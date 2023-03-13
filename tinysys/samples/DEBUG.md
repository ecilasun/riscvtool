```
usbipd wsl list
usbipd.exe wsl attach -d Ubuntu -b 8-2
```

First, make sure to add yourself to the dialout group:

```
sudo gpasswd --add ${USER} dialout
```

To debug, simply type:

```
riscv64-unknown-elf-gdb -b 115200 boot.elf
```