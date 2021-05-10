clang++ --target=riscv32 -march=rv32imf -std=c++11 -fPIC -stdlib++-isystem /usr/lib/llvm-11/include/ -I/usr/include -c test/miniterm.cpp -o miniterm.elf
