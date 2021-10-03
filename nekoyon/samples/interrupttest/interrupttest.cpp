#include "core.h"
#include "uart.h"
#include "encoding.h"
//#include "switches.h"

volatile uint32_t crashhint = 0;
uint32_t seconds = 1;

void software_interrupt()
{
   uint32_t at = read_csr(mtval);
   uint32_t cause = read_csr(mcause);

   // This is an illegal instruction exception
   // If cause&1==0 then it's an ebreak instruction
   if ((cause&1) != 0)
   {
      // Show the address and the failing instruction's opcode field
      UARTWrite("\nEXCEPTION: Illegal instruction D$(0x");
      UARTWriteHex(*(uint32_t*)at);
      UARTWrite(") at 0x");
      UARTWriteHex((uint32_t)at);
      UARTWrite("\n");
   }
   else
   {
      // We've hit a breakpoint
      UARTWrite("\nEXCEPTION: Breakpoint hit (TBD, currently not handled)\n");
   }

   // Deadlock
   while(1) { }
}

void timer_interrupt()
{
    // Output our message
    UARTWrite("TMI: ");
    UARTWriteDecimal(seconds);
    UARTWrite(" seconds\n");

    // At 15 second mark, we'll hit for main() to trigger an illegal instruction exception
    if (seconds == 15)
    {
       UARTWrite("Should crash about now...\n");
       crashhint = 1;
    }

    // Advance time
    ++seconds;

    // NOTE: If one stores registers and restores them to saved registers of a task,
    // a simple task manager can be implemented using timer interrupts only.
    // In that case, it's essential to step the 'future' timer value to a
    // reasonably small value, depending on the task itself. Usually no more than a
    // few milliseconds suffices.

    // Reset the timer interrupt to trigger once more in the future
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
    uint64_t future = now + 10'000'000; // One second into the future
    asm volatile("csrrw zero, 0x801, %0" :: "r" ((future&0xFFFFFFFF00000000)>>32));
    asm volatile("csrrw zero, 0x800, %0" :: "r" (uint32_t(future&0x00000000FFFFFFFF)));
}

void hardware_interrupt(uint16_t device_mask)
{
    // NOTE: It's important to drain the FIFO that triggered the event
    // or we'll keep getting false triggers
    // In this case we'll drain the switches as this sample
    // doesn't expect or cope with incoming UART data or SPI data.

    // Also note that even though the device is de-bouncing the switch
    // inputs, it might be necessary to do a secondary filter in switch/button
    // handling code to track state changes without jitter

    UARTWrite("HWI: devicemask=");
    UARTWriteHex(device_mask);
    // WARNING: MUST read the incoming value from matching device, or fifo will overflow and hang
    if (device_mask&1)
    {
      UARTWrite(" UART[");
      while (*IO_UARTRXByteAvailable)
      {
         UARTWriteDecimal(*IO_UARTRXTX);
         UARTWrite(",");
      }
      UARTWrite("]");
    }
    //UARTWrite(" switches=");
    // while(IO_SwitchDataAvailable)
    //UARTWriteHex(*IO_SwitchRX);
    UARTWrite("\n");
}

void __attribute__((interrupt("machine"))) interrupt_handler()
{
   // NOTE: During the execution of this interrupt handler,
   // no other 'machine' interrupt will fire since interrupt
   // handling will be disabled by hardware until it sees
   // an MRET instruction (return from machine interrupt)
   // This means this code won't be reentrant or execute again
   // until we return from it.
   // This also means that it's better to respond to interrupts
   // as quick as possible to service as many events as possible

   register uint32_t causedword;
   asm volatile("csrr %0, mcause" : "=r"(causedword));
   switch(causedword&0x0000FFFF)
   {
      case 3: // Software exceptions (breakpoint/illegal instruction)
         // Note that this sample turns off these exceptions
         // which are turned on by the ROM at startup.
         software_interrupt();
         break;
      case 7: // Timer events
         timer_interrupt();
         break;
      case 11: // Hardware events
         hardware_interrupt((causedword&0xFFFF0000)>>16);
         break;
      default:
         // NOTE: Unknown interrupts will be ignored
         break;
   }
}

void SetupInterruptHandlers()
{
   // Turn off interrupt handling as was set up by the ROM image
   // to avoid accidents, since we'll poke at handler pointer and masks
   int mstatus = 0;
   asm volatile("csrrw zero, mstatus,%0" :: "r" (mstatus));

   // Set up timer interrupt one second into the future
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
   uint64_t future = now + 10'000'000; // One second into the future
   // NOTE: ALWAYS set high word first to avoid misfires outside timer interrupt
   asm volatile("csrrw zero, 0x801, %0" :: "r" ((future&0xFFFFFFFF00000000)>>32));
   asm volatile("csrrw zero, 0x800, %0" :: "r" (uint32_t(future&0x00000000FFFFFFFF)));

   // Set trap handler address
   asm volatile("csrrw zero, mtvec, %0" :: "r" (interrupt_handler));

   // Enable machine timer and machine external interrupts, as well as software interrupts (ebreak and illegal instruction)
   int msie = (1 << 7) | (1 << 11) | (1 << 3);
   asm volatile("csrrw zero, mie,%0" :: "r" (msie));

   // Enable machine interrupts
   // NOTE: Always enable this last to avoid misfiring events before setup is complete
   mstatus = (1 << 3);
   asm volatile("csrrw zero, mstatus,%0" :: "r" (mstatus));
}

int main()
{
    UARTWrite("Timer interrupt test\n");
    UARTWrite("NOTE: Will force a crash at 15 seconds\n\n");

    SetupInterruptHandlers();

    UARTWrite("Interrupt handlers installed\n\n");

    // At this point, a timer interrupt will be fired every second
    // which will interrupt current work and call the interrupt
    // service routine, then resume current work.

    // Stay here, as we don't have anywhere else to go back to
    while (1) {

      UARTWrite("Sleeping\n");
      // This instruction will wait for an interrupt to occur
      // and won't resume execution until woken up.
      asm volatile("wfi;");

      UARTWrite("Resuming\n");

      // Once resumed, we can check for a forced crash hint
      // for our demo.
      if (crashhint)
      {
        UARTWrite("Deliberately crashing to test handler\n");
        UARTWrite("D$ should show 0x012345FF\n");
        // Emit two illegal instructions to test proper crash handling
        asm volatile(".dword 0x012345FF");
        asm volatile(".dword 0xFFFFFFFF");
      }
    }

    return 0;
}
