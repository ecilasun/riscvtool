#!/bin/sh
echo Building RISC-V ELF files

rm ROMasmdump.txt
rm ROMnekoichiasmdump.txt
rm minitermasmdump.txt
rm gpupipetestasmdump.txt
rm sdcardtestasmdump.txt
rm mandelbrotasmdump.txt
rm rastertestasmdump.txt

rm ROM.coe
rm ROM_nekoichi.coe

# BIOS for ecrv32
riscv64-unknown-elf-gcc -o ROM.elf test/ROM.cpp test/utils.cpp -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,-gc-sections -Wl,--strip-debug -lc -lgcc -Wl,-Ttest/ROM.lds

# BIOS for nekoichi
#riscv64-unknown-elf-gcc -o ROM_nekoichi.elf test/ROM_nekoichi.cpp test/utils.cpp -mexplicit-relocs -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,-gc-sections -Wl,--strip-debug -lc -lm -lgcc -Wl,-Ttest/ROM_nekoichi.lds
riscv64-unknown-elf-g++ -o ROM_nekoichi.elf test/ROM_nekoichi.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -nostartfiles -Wl,-Ttest/ROM_nekoichi.lds

# Examples for ecrv32
riscv64-unknown-elf-g++ -o sdcardtest.elf test/sdcardtest.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -Ofast -march=rv32im -mabi=ilp32 -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc
riscv64-unknown-elf-g++ -o rastertest.elf test/rastertest.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -std=c++11 -Wall -Ofast -march=rv32im -mabi=ilp32 -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -lgcc

# Examples for nekoichi
riscv64-unknown-elf-g++ -o gpupipetest.elf test/gpupipetest.cpp test/utils.cpp -fno-builtin -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc
#riscv64-unknown-elf-g++ -o gpupipetest.elf test/gpupipetest.cpp test/utils.cpp -mexplicit-relocs -std=c++11 -Wall -ffast-math -Ofast -march=rv32imf -mabi=ilp32f -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -Wl,--strip-debug -lc -lm -lgcc -lgloss -Wl,-Ttest/APP_nekoichi.lds
riscv64-unknown-elf-g++ -o miniterm.elf test/miniterm.cpp test/utils.cpp test/SDCARD.cpp test/FAT.cpp -fno-builtin -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc
#riscv64-unknown-elf-g++ -o mandelbrot.elf test/mandelbrot.cpp test/utils.cpp -std=c++11 -Wall -ffast-math -Ofast -march=rv32imf -mabi=ilp32f  -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc
riscv64-unknown-elf-g++ -o mandelbrot.elf test/mandelbrot.cpp test/utils.cpp -mexplicit-relocs -fno-builtin -std=c++11 -Wall -ffast-math -Ofast -march=rv32imf -mabi=ilp32f -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -Wl,--strip-debug -lc -lm -lgcc -lgloss -Wl,-Ttest/APP_nekoichi.lds

# ASM dumps
riscv64-unknown-elf-objdump -d ROM.elf >> ROMasmdump.txt
riscv64-unknown-elf-readelf -S ROM.elf >> ROMasmdump.txt
riscv64-unknown-elf-objdump -d ROM_nekoichi.elf >> ROMnekoichiasmdump.txt
riscv64-unknown-elf-readelf -S ROM_nekoichi.elf >> ROMnekoichiasmdump.txt

riscv64-unknown-elf-objdump -d mandelbrot.elf >> mandelbrotasmdump.txt
riscv64-unknown-elf-readelf -a mandelbrot.elf >> mandelbrotasmdump.txt
riscv64-unknown-elf-objdump -d gpupipetest.elf >> gpupipetestasmdump.txt
riscv64-unknown-elf-readelf -a gpupipetest.elf >> gpupipetestasmdump.txt

riscv64-unknown-elf-objdump -d miniterm.elf >> minitermasmdump.txt
riscv64-unknown-elf-readelf -S miniterm.elf >> minitermasmdump.txt
riscv64-unknown-elf-objdump -d sdcardtest.elf >> sdcardtestasmdump.txt
riscv64-unknown-elf-readelf -S sdcardtest.elf >> sdcardtestasmdump.txt
riscv64-unknown-elf-objdump -d rastertest.elf >> rastertestasmdump.txt
riscv64-unknown-elf-readelf -S rastertest.elf >> rastertestasmdump.txt

# Coefficient files for BIOS update
./build/release/riscvtool ROM.elf -makerom >> ROM.coe
./build/release/riscvtool ROM_nekoichi.elf -makerom >> ROM_nekoichi.coe
