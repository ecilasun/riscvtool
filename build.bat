del dump.txt
del ROM.coe
riscv64-unknown-elf-gcc.exe -o program.elf test/program.cpp -Ofast -fno-tree-loop-distribute-patterns -mexplicit-relocs -march=rv32im -mabi=ilp32 -static -mcmodel=medany -fvisibility=hidden -nostartfiles -fPIC -ffunction-sections -fdata-sections -Wl,-e_start -Wl,-melf32lriscv -Wl,--strip-all -Wl,-gc-sections -lc -lgcc -Wl,-Ttest/program.lds
REM clang.exe test/program.cpp -target riscv32 -march=rv32imf -isystem"C:/Program Files/LLVM/lib/clang/11.0.0/include"
riscv64-unknown-elf-objdump.exe -d program.elf >> dump.txt
riscv64-unknown-elf-readelf.exe -S program.elf >> dump.txt
.\build\release\riscvtool.exe program.elf -makerom >> ROM.coe
