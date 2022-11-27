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

         // Set up global pointer - NOTE: ROM does not use GP
         //"la gp, __global_pointer$;"
         //".option pop;"

         "li sp, 0x2004FFF0;"       // Stack is at end of BRAM (easier to simulate as DDR3 model might return X on reads)

#if defined(MULTIHART_SUPPORT)
         // Set up stack spaces automatically when supporting
         // more than one hardware thread
         // Note that this version leaves one stack space worth of gap at the end
         "la s0, __stack_size$;"    // Grab per-hart stack size from linker script
         "csrr	s1, mhartid;"        // Grab hart id
         "addi s2, s1, 1;"          // Hart id + 1
         "mul s2, s2, s0;"          // stepback = (hartid + 1) * __stack_size;
         "sub sp, sp, s2;"          // stacktop = base - stepback;
         "mv s0, sp;"               // Set frame pointer to current stack pointer

         "bnez s1, workerhartstart;" // Shortcut directly to worker hart entry point (mhartid != 0)
#else
         // Single hardware thread simply needs to use the setup address
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
      // Halt if we ever attempt to exit ROM
      asm (
         "_romfreeze: "
         "j _romfreeze;"
      );
   }
};
