#if defined(__riscv_compressed)
#error ("HALT! The target SoC NekoIchi does not support compressed instruction set!")
#endif

extern "C"
{
   void __attribute__((naked, section (".boot"))) _start()
   {
      asm volatile (

         //".cfi_startproc;"
         //".cfi_undefined ra;"
         ".option push;"
         ".option norelax;"

         // Set up global pointer
         "la gp, __global_pointer$;"

         ".option pop;"

         // Set up stack pointer and align it to 16 bytes
#ifdef STARTUP_ROM
         //"la sp, __stack_top;"
         "li sp, 0x0003FFF0;" // Initial stack lives in Fast RAM
         "mv s0, sp;" // Set frame pointer to same address
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

         // Stop at breakpoint / no return / _exit is useless
         "ebreak;"
      );
   }

   void __attribute__((noreturn, naked, section (".boot"))) _exit(int x)
   {
      /*asm (
         // This will invoke an ECALL to halt the CPU
         "li a1,0;"
         "li a2,0;"
         "li a3,0;"
         "li a4,0;"
         "li a5,0;"
         "li a7,93;"
         "ecall;"
         "j _exit;"
      );*/
   }
};
