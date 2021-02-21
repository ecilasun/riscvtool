del ROMasmdump.txt
del minitermasmdump.txt
del ROM.coe
del miniterm.coe

riscv64-unknown-elf-gcc.exe -o ROM.elf test/ROM.cpp -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32ic -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,--strip-all -Wl,-gc-sections -lc -lgcc -Wl,-Ttest/ROM.lds
riscv64-unknown-elf-objdump.exe -d ROM.elf >> ROMasmdump.txt
riscv64-unknown-elf-readelf.exe -S ROM.elf >> ROMasmdump.txt
.\build\release\riscvtool.exe ROM.elf -makerom >> ROM.coe

riscv64-unknown-elf-gcc.exe -o miniterm.elf test/miniterm.cpp test/umm_malloc.c -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32i -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,--strip-all -Wl,-gc-sections -lc -lgcc -Wl,-Ttest/miniterm.lds
riscv64-unknown-elf-objdump.exe -d miniterm.elf >> minitermasmdump.txt
riscv64-unknown-elf-readelf.exe -S miniterm.elf >> minitermasmdump.txt
.\build\release\riscvtool.exe miniterm.elf -makerom >> miniterm.coe
