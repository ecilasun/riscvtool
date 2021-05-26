# Make sure to compile with -ggdb to see source lines

# upload binary using
# ./build/release/riscvtool ROM_experimental.elf -sendelf 0x10000 /dev/ttyUSB4

riscv64-unknown-elf-gdb -b 115200 ROM_experimental.elf

# After GDB starts, attach and test with:
# target remote /dev/ttyUSB1
# info registers

# NOTE: Use --tui for a graphical UI

# b somebreakpoint
# info reg

# STUB:
# set_debug_traps
# handle_exception
# breakpoint

# If you want GDB to be able to stop your program while it is running,
# you need to use an interrupt-driven serial driver, and arrange for it to stop when
# it receives a ^C (`\003', the control-C character). That is the character which GDB uses to tell the remote system to stop.

