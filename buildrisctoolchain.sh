sudo make clean
./configure --prefix=/opt/riscv --with-multilib-generator="rv32i-ilp32--;rv32ia-ilp32--;rv32ic-ilp32--;rv32imc-ilp32--;rv32im-ilp32--;rv32ima-ilp32--;rv32imfc-ilp32f--;rv32imf-ilp32f--;rv32imaf-ilp32f--"
sudo make
