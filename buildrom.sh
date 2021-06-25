#!/bin/sh
echo Building ROM images...

#rm ROMnekoniasmdump.txt
#rm ROMnekoichiasmdump.txt

rm ROMnekoni.coe
rm ROMnekoichi.coe

# NOTE: Features being developed in ROM_experimental are gradually pulled into these ROMs

# Production ROM for NekoIchi
riscv64-unknown-elf-g++ -o ROMnekoichi.elf -I3rdparty/ ROMnekoichi/rom.cpp nekolib/nekoichi.cpp -std=c++11 -Wall -Ofast -march=rv32imf -mabi=ilp32f -ffunction-sections -fdata-sections -Wl,-gc-sections,--strip-debug --specs=nano.specs -fPIC -lgcc -nostartfiles -Wl,-TROMnekoichi/rom.lds

# Production ROM for NekoNi
riscv64-unknown-elf-g++ -o ROMnekoni.elf -I3rdparty/ ROMnekoni/rom.cpp nekolib/nekoichi.cpp -std=c++11 -Wall -Ofast -march=rv32e -mabi=ilp32e -ffunction-sections -fdata-sections -Wl,-gc-sections,--strip-debug --specs=nano.specs -fPIC -lgcc -nostartfiles -Wl,-TROMnekoni/rom.lds

# ASM dumps - for debug purposes only (not a requirement to have them)
#riscv64-unknown-elf-objdump -d -t -r  ROMnekoichi.elf >> ROMnekoichiasmdump.txt
#riscv64-unknown-elf-readelf -a ROMnekoichi.elf >> ROMnekoichiasmdump.txt
#riscv64-unknown-elf-objdump -d -t -r  ROMnekoni.elf >> ROMnekoniasmdump.txt
#riscv64-unknown-elf-readelf -a ROMnekoni.elf >> ROMnekoniasmdump.txt

# Coefficient files for ROM update and the exterimental ROM
./build/release/riscvtool ROMnekoni.elf -makerom 0 0 >> ROMnekoni.coe
./build/release/riscvtool ROMnekoichi.elf -makerom 0 0 >> ROMnekoichi.coe
