riscv64-unknown-elf-gdb -b 115200
# target remote /dev/ttyUSB1
# file something.elf
# load
# b somebreakpoint
# info reg

# STUB:
# set_debug_traps
# handle_exception
# breakpoint

# If you want GDB to be able to stop your program while it is running,
# you need to use an interrupt-driven serial driver, and arrange for it to stop when
# it receives a ^C (`\003', the control-C character). That is the character which GDB uses to tell the remote system to stop.

