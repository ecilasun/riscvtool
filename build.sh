#!/bin/sh
echo Building RISC-V ELF files

rm ROMnekoichiasmdump.txt
rm ROMexperimentalasmdump.txt
rm minitermasmdump.txt
rm gpupipetestasmdump.txt
rm mandelbrotasmdump.txt

rm ROM_nekoichi.coe
rm ROM_experimental.coe

# BIOS for nekoichi
#riscv64-unknown-elf-gcc -o ROM_nekoichi.elf test/ROM_nekoichi.cpp test/utils.cpp -mexplicit-relocs -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,-gc-sections -Wl,--strip-debug -lc -lm -lgcc -Wl,-Ttest/ROM_nekoichi.lds
riscv64-unknown-elf-g++ -o ROM_nekoichi.elf test/ROM_nekoichi.cpp test/utils.cpp -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -nostartfiles -Wl,-Ttest/ROM_nekoichi.lds
riscv64-unknown-elf-g++ -o ROM_experimental.elf test/ROM_experimental.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp test/diskio.cpp test/console.cpp -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -nostartfiles -Wl,-Ttest/ROM_nekoichi.lds

# Examples for nekoichi
riscv64-unknown-elf-g++ -o gpupipetest.elf test/gpupipetest.cpp test/utils.cpp test/console.cpp -fno-builtin -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm
riscv64-unknown-elf-g++ -o miniterm.elf test/miniterm.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp test/diskio.cpp test/console.cpp -fno-builtin -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm
riscv64-unknown-elf-g++ -o mandelbrot.elf test/mandelbrot.cpp test/utils.cpp -std=c++11 -Wall -ffast-math -Ofast -fno-builtin -mcmodel=medany -march=rv32imf -mabi=ilp32f  -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgloss -lm

# ASM dumps
riscv64-unknown-elf-objdump -d -t -r  ROM_nekoichi.elf >> ROMnekoichiasmdump.txt
riscv64-unknown-elf-readelf -a ROM_nekoichi.elf >> ROMnekoichiasmdump.txt
riscv64-unknown-elf-readelf -a ROM_experimental.elf >> ROMexperimentalasmdump.txt

riscv64-unknown-elf-objdump -d -t -r mandelbrot.elf >> mandelbrotasmdump.txt
riscv64-unknown-elf-readelf -a mandelbrot.elf >> mandelbrotasmdump.txt
riscv64-unknown-elf-objdump -d -t -r gpupipetest.elf >> gpupipetestasmdump.txt
riscv64-unknown-elf-readelf -a gpupipetest.elf >> gpupipetestasmdump.txt
riscv64-unknown-elf-objdump -d -t -r miniterm.elf >> minitermasmdump.txt
riscv64-unknown-elf-readelf -a miniterm.elf >> minitermasmdump.txt

# Coefficient files for BIOS update
./build/release/riscvtool ROM_nekoichi.elf -makerom >> ROM_nekoichi.coe
./build/release/riscvtool ROM_experimental.elf -makerom >> ROM_experimental.coe
