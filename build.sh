#!/bin/sh
echo Building RISC-V ELF files

rm ROMasmdump.txt
rm minitermasmdump.txt
rm instructiontestasmdump.txt
rm sdcardtestasmdump.txt
rm mandelbrotasmdump.txt
rm rastertestasmdump.txt

rm ROM.coe

# BIOS
riscv64-unknown-elf-gcc -o ROM.elf test/ROM.cpp test/utils.cpp -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,-gc-sections -Wl,--strip-debug -lc -lgcc -Wl,-Ttest/ROM.lds

# Examples
riscv64-unknown-elf-g++ -o miniterm.elf test/miniterm.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -g -Ofast -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc
riscv64-unknown-elf-g++ -o instructiontest.elf test/instructiontest.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -g -Ofast -march=rv32im -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc
riscv64-unknown-elf-g++ -o sdcardtest.elf test/sdcardtest.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -g -Ofast -march=rv32im -mabi=ilp32 -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc
riscv64-unknown-elf-g++ -o mandelbrot.elf test/mandelbrot.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -g -Ofast -march=rv32im -mabi=ilp32 -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc
riscv64-unknown-elf-g++ -o rastertest.elf test/rastertest.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -g -Ofast -march=rv32im -mabi=ilp32 -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc

# ASM dumps
riscv64-unknown-elf-objdump -d ROM.elf >> ROMasmdump.txt
riscv64-unknown-elf-readelf -S ROM.elf >> ROMasmdump.txt
riscv64-unknown-elf-objdump -d miniterm.elf >> minitermasmdump.txt
riscv64-unknown-elf-readelf -S miniterm.elf >> minitermasmdump.txt
riscv64-unknown-elf-objdump -d instructiontest.elf >> instructiontestasmdump.txt
riscv64-unknown-elf-readelf -S instructiontest.elf >> instructiontestasmdump.txt
riscv64-unknown-elf-objdump -d sdcardtest.elf >> sdcardtestasmdump.txt
riscv64-unknown-elf-readelf -S sdcardtest.elf >> sdcardtestasmdump.txt
riscv64-unknown-elf-objdump -d mandelbrot.elf >> mandelbrotasmdump.txt
riscv64-unknown-elf-readelf -S mandelbrot.elf >> mandelbrotasmdump.txt
riscv64-unknown-elf-objdump -d rastertest.elf >> rastertestasmdump.txt
riscv64-unknown-elf-readelf -S rastertest.elf >> rastertestasmdump.txt

# Coefficient file for BIOS update
./build/release/riscvtool ROM.elf -makerom >> ROM.coe
