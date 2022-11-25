#if defined(__riscv_compressed)
#error ("HALT! The target SoC does not support compressed instruction set!")
#endif

// This unit has more than one HART
// Defined this to allow for auto-setup of per-HART stacks
#define MULTIHART_SUPPORT

extern "C"
{
   void __attribute__((naked, section (".boot"))) _start()
   {
      asm volatile (

         // This is the entry point of CORE#0 (0x10000020 if above code is enabled)
         //".cfi_startproc;"
         //".cfi_undefined ra;"
         //".option push;"
         //".option norelax;"

         // Set up global pointer
         //"la gp, __global_pointer$;"
         //".option pop;"

#if defined(MULTIHART_SUPPORT)
         "li sp, 0x1FFFFFF0;"       // End of memory (SDRAM)
         "la s0, __stack_size$;"    // Grab per-hart stack size from linker script
         "csrr	s1, mhartid;"        // Grab hart id
         "addi s2, s1, 1;"          // Hart id + 1
         "mul s2, s2, s0;"          // stepback = (hartid + 1) * __stack_size;
         "sub sp, sp, s2;"          // stacktop = base - stepback;
         "mv s0, sp;"               // Set frame pointer to current stack pointer

         "bnez s1, workerhartstart;" // Shortcut directly to worker hart entry point (mhartid != 0)
#else
         "li sp, 0x1FFFFFF0;"       // End of memory (SDRAM)
         "la s0, __stack_size$;"    // stepback = __stack_size$;
         "sub sp, sp, s0;"          // stacktop = base - stepback;
         "mv s0, sp;"               // Set frame pointer to current stack pointer
#endif

         // Clear BSS - NOTE: can skip for hardware debug builds
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

#if defined(MULTIHART_SUPPORT)
         "workerhartstart:"   // Entry point for extra harts (hartid!=0)

         "lw a0,0(sp);"
         "addi	a1,sp,4;"
         "li a2,0;"

         "j _Z10workermainv;"

         // Stop at breakpoint / no return / _exit is useless
         "ebreak;"
#endif
      );
   }

   void __attribute__((noreturn, naked, section (".boot"))) _exit(int x)
   {
      /*asm (
         // This will invoke an ECALL to halt the HART
         "li a1,0x1;"
         "li a2,0x2;"
         "li a3,0x3;"
         "li a4,0x4;"
         "li a5,0x5;"
         "li a7,93;"
         "ecall;"
         "j _exit;"
      );*/
   }
};
