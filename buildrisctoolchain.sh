# On Ubuntu, run the following to grab the toolchain:
git clone https://github.com/riscv/riscv-gnu-toolchain

# Install the prerequisites
sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build

# Optional - If you have something left from a prior session, run:
# sudo make clean

# Next step is to configure for the variants of the RISC-V chip you're going to use
./configure --prefix=/opt/riscv --with-multilib-generator="rv32i-ilp32--;rv32ia-ilp32--;rv32ic-ilp32--;rv32imc-ilp32--;rv32im-ilp32--;rv32ima-ilp32--;rv32imfc-ilp32f--;rv32imf-ilp32f--;rv32imaf-ilp32f--"

# And finally, build it
sudo make

# When done, add /opt/riscv/bin to PATH
export PATH=/opt/riscv/bin:$PATH
