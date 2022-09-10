#if defined(__riscv_compressed)
#error ("HALT! The target SoC does not support compressed instruction set!")
#endif

// This unit has more than one HART
// Defined this to allow for auto-setup of per-HART stacks
#define MULTIHART

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

#if defined(MULTIHART)
         "csrr	s1, mhartid;"        // get hart id
         "slli	s1, s1, 8;"          // hartid*256 (256b (2^8) default stack)
         "li s2, 0x0000FE00;"       // stack top of last HART in BMEM (near end of 64KBytes)
         "sub s2, s2, s1;"          // base - hartid*256
         "mv sp, s2;"               // set new hart stack pointer
         "mv s0, sp;"               // set frame pointer

         "bnez s1, workerhartstart;" // Shortcut directly to worker hart entry point (mhartid != 0)
#else
         "li sp, 0x0000FE00;"        // single hart, hardcoded stack at end of BMEM (512 bytes)
         "mv s0, sp;"                // set frame pointer
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

#if defined(MULTIHART)
         "workerhartstart:"

         "lw a0,0(sp);"
         "addi	a1,sp,4;"
         "li a2,0;"

         "j _Z10workermainv;" // All HARTs with id!=0 fall here

         // Stop at breakpoint / no return / _exit is useless
         "ebreak;"
#endif
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
