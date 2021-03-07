del ROMasmdump.txt
del minitermasmdump.txt
del instructiontestasmdump.txt
del ROM.coe
del miniterm.coe
del instructiontest.coe

riscv64-unknown-elf-gcc.exe -o ROM.elf test/ROM.cpp -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,-gc-sections -Wl,--strip-debug -lc -lgcc -Wl,-Ttest/ROM.lds
riscv64-unknown-elf-objdump.exe -d ROM.elf >> ROMasmdump.txt
riscv64-unknown-elf-readelf.exe -S ROM.elf >> ROMasmdump.txt
.\build\release\riscvtool.exe ROM.elf -makerom >> ROM.coe

riscv64-unknown-elf-g++.exe -o miniterm.elf test/miniterm.cpp -Ofast -march=rv32imc -mabi=ilp32 -ffreestanding -fno-common -static -mcmodel=medany -fvisibility=hidden -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,-gc-sections -Wl,--strip-debug -lc -lgcc -Wl,-Ttest/APP.lds
riscv64-unknown-elf-objdump.exe -d miniterm.elf >> minitermasmdump.txt
riscv64-unknown-elf-readelf.exe -S miniterm.elf >> minitermasmdump.txt
.\build\release\riscvtool.exe miniterm.elf -makerom >> miniterm.coe

riscv64-unknown-elf-g++.exe -o instructiontest.elf test/instructiontest.cpp -Ofast -fno-use-cxa-atexit -fno-exceptions -fno-rtti -march=rv32imc -mabi=ilp32 -ffreestanding -fno-common -static -fvisibility=hidden -fPIC -ffunction-sections -fdata-sections -Wl,-gc-sections -Wl,--strip-debug -lc -lgcc -Wl,-Ttest/APP.lds
riscv64-unknown-elf-objdump.exe -d instructiontest.elf >> instructiontestasmdump.txt
riscv64-unknown-elf-readelf.exe -S instructiontest.elf >> instructiontestasmdump.txt
.\build\release\riscvtool.exe instructiontest.elf -makerom >> instructiontest.coe


REM -ffreestanding -fstack-protector-strong
