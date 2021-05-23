#if defined(__riscv_compressed)
#error ("HALT! The target SoC NekoIchi does not support compressed instruction set!")
#endif

#ifdef EXPERIMENTAL_ROM
// Wall clock runs at 10Mhz which is 10 million ticks per second
#define ONE_SECOND_IN_TICKS 10000000
// 10ms intervals for task switching
//#define TASK_TIMESLICE 100000
// 1ms for rapid testing
#define TASK_TIMESLICE 10000
// Interrupt handlers
extern "C"
{
   extern void timer_interrupt();
   extern void breakpoint_interrupt();
   extern void external_interrupt();
}
#endif // EXPERIMENTAL_ROM

extern "C"
{
   void __attribute__((naked, section (".boot"))) _start()
   {

#ifdef STARTUP_ROM
      asm volatile (
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
      );
#endif

#ifdef EXPERIMENTAL_ROM

      uint32_t clockhigh, clocklow, tmp;
      asm volatile(
         "1:\n"
         "rdtimeh %0\n"
         "rdtime %1\n"
         "rdtimeh %2\n"
         "bne %0, %2, 1b\n"
         : "=&r" (clockhigh), "=&r" (clocklow), "=&r" (tmp)
      );
      uint64_t now = (uint64_t(clockhigh)<<32) | clocklow;
      uint64_t future = now + TASK_TIMESLICE; // Task switching frequency
#endif // EXPERIMENTAL_ROM

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
         "li x12, 0x0003FFF0;"
         "mv sp, x12;"
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

#ifdef EXPERIMENTAL_ROM
         // Set trap handler address
         "la t0, trap_handler;"
         "csrrw zero, mtvec, t0;"

         "csrrw zero, 0x801, %0;" 
         "csrrw zero, 0x800, %1;"

         // Enable machine interrupts
         "li t0, 8;" // (1 << 3)
         "csrrw zero, mstatus, t0;"

         // Enable machine timer interrupts and machine external interrupts
         "li t0, 2176;" // (1 << 7) | (1 << 11)
         "csrrw zero, mie, t0;"
#endif // EXPERIMENTAL_ROM
         // Jump to main
         "j main;"

         // Stop at breakpoint / no return / _exit is useless
         "ebreak;"

#ifdef EXPERIMENTAL_ROM
     "trap_handler:\n"

         "addi sp,sp,-144;"
         "sw	ra,140(sp);"
         "sw	t0,136(sp);"
         "sw	t1,132(sp);"
         "sw	t2,128(sp);"
         "sw	a0,124(sp);"
         "sw	a1,120(sp);"
         "sw	a2,116(sp);"
         "sw	a3,112(sp);"
         "sw	a4,108(sp);"
         "sw	a5,104(sp);"
         "sw	a6,100(sp);"
         "sw	a7,96(sp);"
         "sw	t3,92(sp);"
         "sw	t4,88(sp);"
         "sw	t5,84(sp);"
         "sw	t6,80(sp);"
         "fsw	ft0,76(sp);"
         "fsw	ft1,72(sp);"
         "fsw	ft2,68(sp);"
         "fsw	ft3,64(sp);"
         "fsw	ft4,60(sp);"
         "fsw	ft5,56(sp);"
         "fsw	ft6,52(sp);"
         "fsw	ft7,48(sp);"
         "fsw	fa0,44(sp);"
         "fsw	fa1,40(sp);"
         "fsw	fa2,36(sp);"
         "fsw	fa3,32(sp);"
         "fsw	fa4,28(sp);"
         "fsw	fa5,24(sp);"
         "fsw	fa6,20(sp);"
         "fsw	fa7,16(sp);"
         "fsw	ft8,12(sp);"
         "fsw	ft9,8(sp);"
         "fsw	ft10,4(sp);"
         "fsw	ft11,0(sp);"

         "csrr t0, mcause;"
         "li t1, 7;"
         "bne	t0,t1,skiptimer;"
         "jal ra, timer_interrupt;"
         "j restoreregisters;"

      "skiptimer:"
         "li t1, 3;"
         "bne	t0,t1,skipbreakpoint;"
         "jal ra, breakpoint_interrupt;"
         "j restoreregisters;"

      "skipbreakpoint:"
         "li t1, 11;"
         "bne	t0,t1,restoreregisters;"
         "jal ra, external_interrupt;"
      
      "restoreregisters:"
         "lw	ra,140(sp);"
         "lw	t0,136(sp);"
         "lw	t1,132(sp);"
         "lw	t2,128(sp);"
         "lw	a0,124(sp);"
         "lw	a1,120(sp);"
         "lw	a2,116(sp);"
         "lw	a3,112(sp);"
         "lw	a4,108(sp);"
         "lw	a5,104(sp);"
         "lw	a6,100(sp);"
         "lw	a7,96(sp);"
         "lw	t3,92(sp);"
         "lw	t4,88(sp);"
         "lw	t5,84(sp);"
         "lw	t6,80(sp);"
         "flw	ft0,76(sp);"
         "flw	ft1,72(sp);"
         "flw	ft2,68(sp);"
         "flw	ft3,64(sp);"
         "flw	ft4,60(sp);"
         "flw	ft5,56(sp);"
         "flw	ft6,52(sp);"
         "flw	ft7,48(sp);"
         "flw	fa0,44(sp);"
         "flw	fa1,40(sp);"
         "flw	fa2,36(sp);"
         "flw	fa3,32(sp);"
         "flw	fa4,28(sp);"
         "flw	fa5,24(sp);"
         "flw	fa6,20(sp);"
         "flw	fa7,16(sp);"
         "flw	ft8,12(sp);"
         "flw	ft9,8(sp);"
         "flw	ft10,4(sp);"
         "flw	ft11,0(sp);"
         "addi	sp,sp,144;"
         "mret;"

         :: "r" ((future&0xFFFFFFFF00000000)>>32), "r" (uint32_t(future&0x00000000FFFFFFFF))
#endif // EXPERIMENTAL_ROM
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
