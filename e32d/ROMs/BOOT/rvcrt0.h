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

         // Set up stack pointer and align it to 16 bytes
         //"la sp, __stack_top;"
         "li sp, 0x8000FFF0;" // HART#0 stack
         "mv s0, sp;"

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

   void __attribute__((noreturn, naked, section (".workerinit"))) _workerstart()
   {
      asm volatile (

         // TODO: Wait for HART#0 to wake us up
         // This could be done via a memory mapped mailbox read which initially contains 0x000000001 (HART#0 enabled)
         // We wait in an infinite loop to see if this HART's corresponding bit is set to 1 then resume execution.
         // Otherwise, the memset etc code above will kick and write zeroes over memory we're using without this synchronization.
         // Another memory mapped register could be used for a polling mechanism to see if a certain task is 'done' by
         // all CPUs in the same work group (as with a GPU)

         ".option push;"
         ".option norelax;"

         // Set up global pointer
         "la gp, __global_pointer$;"

         ".option pop;"

         "li sp, 0x8000F000;" // HART#1 stack
         "mv s0, sp;"

         "lw a0,0(sp);"
         "addi	a1,sp,4;"
         "li a2,0;"

         "j _Z10workermainv;"
         "ebreak;"
      );
   }
};
