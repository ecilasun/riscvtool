del ROMasmdump.txt
del ROM.coe

riscv64-unknown-elf-gcc.exe -o ROM.elf test/ROM.cpp -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32im -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,--strip-all -Wl,-gc-sections -lc -lgcc -Wl,-Ttest/ROM.lds
riscv64-unknown-elf-objdump.exe -d ROM.elf >> ROMasmdump.txt
riscv64-unknown-elf-readelf.exe -S ROM.elf >> ROMasmdump.txt
.\build\release\riscvtool.exe ROM.elf -makerom >> ROM.coe

riscv64-unknown-elf-gcc.exe -o miniterm.elf test/miniterm.cpp -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32im -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,--strip-all -Wl,-gc-sections -lc -lgcc -Wl,-Ttest/miniterm.lds
