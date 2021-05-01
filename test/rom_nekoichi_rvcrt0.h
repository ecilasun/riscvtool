#if defined(__riscv_compressed)
#error ("HALT! The target SoC NekoIchi does not support compressed instruction set!")
#endif

extern "C"
{
   void __attribute__((naked, section (".boot"))) _start()
   {
      asm (
#ifdef STARTUP_ROM
         "mv  x1, x0;"
         "mv  x2, x1;"
         "mv  x3, x1;"
         "mv  x4, x1;"
         "mv  x5, x1;"
         "mv  x6, x1;"
         "mv  x7, x1;"
         "mv  x8, x1;"
         "mv  x9, x1;"
         "mv x10, x1;"
         "mv x11, x1;"
         "mv x12, x1;"
         "mv x13, x1;"
         "mv x14, x1;"
         "mv x15, x1;"
         "mv x16, x1;"
         "mv x17, x1;"
         "mv x18, x1;"
         "mv x19, x1;"
         "mv x20, x1;"
         "mv x21, x1;"
         "mv x22, x1;"
         "mv x23, x1;"
         "mv x24, x1;"
         "mv x25, x1;"
         "mv x26, x1;"
         "mv x27, x1;"
         "mv x28, x1;"
         "mv x29, x1;"
         "mv x30, x1;"
         "mv x31, x1;"
         "fmv.w.x	f0, zero;"
         "fmv.w.x	f1, zero;"
         "fmv.w.x	f2, zero;"
         "fmv.w.x	f3, zero;"
         "fmv.w.x	f4, zero;"
         "fmv.w.x	f5, zero;"
         "fmv.w.x	f6, zero;"
         "fmv.w.x	f7, zero;"
         "fmv.w.x	f8, zero;"
         "fmv.w.x	f9, zero;"
         "fmv.w.x	f10, zero;"
         "fmv.w.x	f11, zero;"
         "fmv.w.x	f12, zero;"
         "fmv.w.x	f13, zero;"
         "fmv.w.x	f14, zero;"
         "fmv.w.x	f15, zero;"
         "fmv.w.x	f16, zero;"
         "fmv.w.x	f17, zero;"
         "fmv.w.x	f18, zero;"
         "fmv.w.x	f19, zero;"
         "fmv.w.x	f20, zero;"
         "fmv.w.x	f21, zero;"
         "fmv.w.x	f22, zero;"
         "fmv.w.x	f23, zero;"
         "fmv.w.x	f24, zero;"
         "fmv.w.x	f25, zero;"
         "fmv.w.x	f26, zero;"
         "fmv.w.x	f27, zero;"
         "fmv.w.x	f28, zero;"
         "fmv.w.x	f29, zero;"
         "fmv.w.x	f30, zero;"
         "fmv.w.x	f31, zero;"
#endif

         //".cfi_startproc;"
         //".cfi_undefined ra;"
         ".option push;"
         ".option norelax;"

         // Set up global pointer
         "la gp, __global_pointer$;"

         ".option pop;"

         // Set up stack pointer and align it to 16 bytes
#ifdef STARTUP_ROM
         "la sp, __stack_top;"
         "add s0, sp, zero;"
#endif

         // Clear BSS
         "la a0, __malloc_max_total_mem;"
         "la a2, __BSS_END__$;"
         "sub a2, a2, a0;"
         "li a1, 0;"
         "jal ra, memset;"

         // Skip if there's no atexit function
         "la a0, atexit;"
         "beqz a0, noatexitfound;"

         // Register finiarray with atexit
         "auipc	a0,0x1;"
         "la a0, __libc_fini_array;"
         "jal ra, atexit;"

         // call initarray
         "noatexitfound: "
         "jal ra, __libc_init_array;"

         "lw a0,0(sp);"
         "addi	a1,sp,4;"
         "li a2,0;"

         // Jump to main
         "j main;"

         // No return
         //"j	_exit;"
         //".cfi_endproc;"
         //".end;"
      );
   }

   void __attribute__((noreturn, naked, section (".boot"))) _exit(int x)
   {
      asm (
         // This will invoke an ECALL to halt the CPU
         "li a1,0;"
         "li a2,0;"
         "li a3,0;"
         "li a4,0;"
         "li a5,0;"
         "li a7,93;"
         "ecall;"
         "j _exit;"
      );
   }
};
