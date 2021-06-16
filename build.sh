#!/bin/sh
echo Building RISC-V ELF files

rm ROMnekoniasmdump.txt
rm ROMnekoichiasmdump.txt
rm ROMexperimentalasmdump.txt

#rm ddrtestasmdump.txt
#rm rainsynthasmdump.txt
#rm minitermasmdump.txt
#rm gpupipetestasmdump.txt
rm mandelbrotasmdump.txt

rm ROM_nekoni.coe
rm ROM_nekoichi.coe
rm ROM_experimental.coe

# Production ROM for NekoIchi
# ROM has the UART loader which can accept and run binaries via riscvtool, and nothing much else
# Features being developed in ROM_experimental are gradually pulled into this ROM code
riscv64-unknown-elf-g++ -o ROM_nekoichi.elf test/ROM_nekoichi.cpp test/nekoichi.cpp -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -nostartfiles -Wl,-Ttest/ROM_nekoichi.lds
riscv64-unknown-elf-g++ -o ROM_nekoni.elf test/ROM_nekoni.cpp test/nekoichi.cpp -std=c++11 -Wall -Ofast -march=rv32e -mabi=ilp32e -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -nostartfiles -Wl,-Ttest/ROM_nekoni.lds

# Experimental ROM for NekoIchi
# Experimental ROM contains test code and is mainly used for kernel/library development
# Currently working on threading & interrupt handlers
#riscv64-unknown-elf-g++ -o ROM_experimental.elf test/ROM_experimental.cpp test/debugger.cpp test/nekoichi.cpp test/sdcard.cpp test/pff.cpp test/diskio.cpp test/console.cpp -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -nostartfiles -Wl,-Ttest/ROM_experimental.lds
# NOTE: this one will generate line debug/line number information (-g)
riscv64-unknown-elf-g++ -o ROM_experimental.elf test/ROM_experimental.cpp test/debugger.cpp test/nekoichi.cpp test/sdcard.cpp test/pff.cpp test/diskio.cpp test/console.cpp -fno-builtin -mcmodel=medany -std=c++11 -Wall -Ofast -g -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm

# Examples for nekoichi
# Each one shows or test a certain functionality and serve as how-to examples
riscv64-unknown-elf-g++ -o rainsynth.elf test/rainsynth.cpp test/nekoichi.cpp -fno-builtin -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm
riscv64-unknown-elf-g++ -o ddrtest.elf test/ddrtest.cpp test/nekoichi.cpp test/sdcard.cpp test/pff.cpp test/diskio.cpp -fno-builtin -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm
riscv64-unknown-elf-g++ -o gpupipetest.elf test/gpupipetest.cpp test/nekoichi.cpp test/console.cpp -fno-builtin -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm
riscv64-unknown-elf-g++ -o miniterm.elf test/miniterm.cpp test/nekoichi.cpp test/sdcard.cpp test/pff.cpp test/diskio.cpp test/console.cpp -fno-builtin -mcmodel=medany -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgcc -lm
riscv64-unknown-elf-g++ -o mandelbrot.elf test/mandelbrot.cpp test/nekoichi.cpp test/sdcard.cpp test/pff.cpp test/diskio.cpp -std=c++11 -Wall -ffast-math -Ofast -fno-builtin -mcmodel=medany -march=rv32imf -mabi=ilp32f  -ffunction-sections -fdata-sections -Wl,-gc-sections -fPIC -lgloss -lm

# ASM dumps - for debug purposes only (not a requirement to have them)
riscv64-unknown-elf-objdump -d -t -r  ROM_nekoichi.elf >> ROMnekoichiasmdump.txt
riscv64-unknown-elf-readelf -a ROM_nekoichi.elf >> ROMnekoichiasmdump.txt
riscv64-unknown-elf-objdump -d -t -r  ROM_nekoichi.elf >> ROMnekoniasmdump.txt
riscv64-unknown-elf-readelf -a ROM_nekoichi.elf >> ROMnekoniasmdump.txt

riscv64-unknown-elf-objdump -d -t -r  ROM_experimental.elf >> ROMexperimentalasmdump.txt
riscv64-unknown-elf-readelf -a ROM_experimental.elf >> ROMexperimentalasmdump.txt

#riscv64-unknown-elf-objdump -d -t -r ddrtest.elf >> ddrtestasmdump.txt
#riscv64-unknown-elf-readelf -a ddrtest.elf >> ddrtestasmdump.txt
#riscv64-unknown-elf-objdump -d -t -r rainsynth.elf >> rainsynthasmdump.txt
#riscv64-unknown-elf-readelf -a rainsynth.elf >> rainsynthasmdump.txt
riscv64-unknown-elf-objdump -d -t -r mandelbrot.elf >> mandelbrotasmdump.txt
riscv64-unknown-elf-readelf -a mandelbrot.elf >> mandelbrotasmdump.txt
#riscv64-unknown-elf-objdump -d -t -r gpupipetest.elf >> gpupipetestasmdump.txt
#riscv64-unknown-elf-readelf -a gpupipetest.elf >> gpupipetestasmdump.txt
#riscv64-unknown-elf-objdump -d -t -r miniterm.elf >> minitermasmdump.txt
#riscv64-unknown-elf-readelf -a miniterm.elf >> minitermasmdump.txt

# Coefficient files for ROM update and the exterimental ROM
./build/release/riscvtool ROM_nekoni.elf -makerom 0 0 >> ROM_nekoni.coe
./build/release/riscvtool ROM_nekoichi.elf -makerom 0 0 >> ROM_nekoichi.coe
./build/release/riscvtool ROM_experimental.elf -makerom 0 0 >> ROM_experimental.coe
