/*#if defined(__riscv_compressed)
#error ("HALT! The target SoC does not support compressed instruction set!")
#endif*/

extern "C"
{
   void __attribute__((naked, section (".boot"))) _start()
   {
      asm (
         "li a1,0;"
         "li a2,0;"
         "li a3,0;"
         "li a4,0;"
         "li a5,0;"
         "li a7,0;"
         "la gp, __global_pointer$;"
         "la sp, __stack;"
         "add s0, sp, zero;"
         "call main;"
         "j _exit;"
      );
   }

   void __attribute__((noreturn, naked, section (".boot"))) _exit(int x)
   {
      asm (
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
