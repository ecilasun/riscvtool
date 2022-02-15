#if defined(__riscv_compressed)
#error ("HALT! The target SoC does not support compressed instruction set!")
#endif

extern "C"
{
   void __attribute__((naked, section (".boot"))) _start()
   {
      asm volatile (

         // This is the entry point of CORE#0 (0x10000020 if above code is enabled)
         //".cfi_startproc;"
         //".cfi_undefined ra;"
         ".option push;"
         ".option norelax;"

         // Set up global pointer
         "la gp, __global_pointer$;"
         ".option pop;"

         "csrr	s1, mhartid;"       // get hart id
         "slli	s1, s1, 12;"        // hartid*4096 (4K default stack)
         "li s2, 0x0000C000;"      // stack pointer of last HART
         "add s2, s2, s1;"         // base + hartid*4096
         "li s3, 0x80000000;"      // base of BRAM
         "or s2, s2, s3;"          // move into BRAM space
         "mv sp, s2;"              // set new hart stack pointer
         "mv s0, sp;"

         "bnez s1, workerhartstart;" // Shortcut directly to worker hart entry point 

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

         "workerhartstart:"
         "j _Z10workermainv;" // All HARTs with id!=0 fall here
         "ebreak;"
      );
   }

   void __attribute__((noreturn, naked, section (".boot"))) _exit(int x)
   {
      /*asm (
         // This will invoke an ECALL to halt the HART
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
