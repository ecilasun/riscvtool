// Boot ROM

#if defined(__riscv_compressed)
#error ("HALT! The target SoC does not support compressed instruction set!")
#endif

// This module contains instruction sequences for certain operations that will be inserted by the FETCH unit
// For instance, when an interrupt is encountered, enter*ISR() will be invoked and PC will be set to MTRAP

extern "C"
{
    // MIP:
    // Interrupt pending register for local interrupts when using CLINT modes of operation. In CLIC modes, this is hardwired to 0 and pending interrupts are handled using clicintip[i] memory mapped registers
    // Machine external                  Internal timer                  Local software
    // MEIP[11] WIRI[10] SEIP[9] UEIP[8] MTIP[7] WIRI[6] STIP[5] UTIP[4] MSIP[3] WIRI[2] SSIP[1] USIP[0]
    //
    // MIE:
    // Interrupt enable register for local interrupts when using CLINT modes of operation. In CLIC modes, this is hardwired to 0 and interrupt enables are handled using clicintie[i] memory mapped registers
    // Machine external                  Internal timer                  Local software
    // MEIE[11] WPRI[10] SEIE[9] UEIE[8] MTIE[7] WPRI[6] STIP[5] UTIE[4] MSIE[3] WPRI[2] SSIE[1] USIE[0]
    //
    // MSTATUS:
    // Status register containing interrupt enables for all privilege modes, previous privilege mode, and other privilege level setting
    //                     Privilege stack              Enable stack                    Per-privilege global enable
    // XS[16:15] FS[14:13] MPP[12:11] WPRI[10:9] SPP[8] MPIE[7] WPRI[6] SPIE[5] UPIE[4] MIE[3] WPRI[2] SIE[1] UIE[0]
    //
    // MCAUSE:
    // Status register which indicates whether an exception or interrupt occurred, along with a code to distinguish details of each type.
    // IRQ or Exception  Exception code
    // ISEXCEPTION[31]   WLRL[31:0]
    //
    // MTVEC:
    // Machine Trap Vector register which holds the base address of the interrupt vector table, as well as the interrupt mode configuration (direct or vectored) for CLINT and CLIC controllers. All synchronous exceptions also use mtvec as the base address for exception handling in all CLINT and CLIC modes.
    // BASE[31:2] MODE[1:0] (modes-> 00:direct, 01:vectored, 10/11:reserved)
    // Vectored interrupts (exceptions jump to BASE, interrupts jump to BASE+excode*4)
    //
    // Interrupt codes
    // int excode   description
    // 1   0        user software interrupt
    // 1   1        supervisor software interrupt
    // 1   2        reserved
    // 1   3        machine software interrupt
    // 1   4        user timer interrupt
    // 1   5        supervisor timer interrupt
    // 1   6        reserved
    // 1   7        machine timer interrupt
    // 1   8        user external interrupt
    // 1   9        supervisor external interrupt
    // 1   10       reserved
    // 1   11       machine external interrupt
    // 1   12>=     reserved
    //
    // Cause codes
    // Interrupt	Exception Code	Description	
    // 1	0	Reserved	
    // 1	1	Supervisor software interrupt	
    // 1	2	Reserved	
    // 1	3	Machine software interrupt	
    // 1	4	Reserved	
    // 1	5	Supervisor timer interrupt	
    // 1	6	Reserved	
    // 1	7	Machine timer interrupt	
    // 1	8	Reserved	
    // 1	9	Supervisor external interrupt	
    // 1	10	Reserved	
    // 1	11	Machine external interrupt	
    // 1	12–15	Reserved	
    // 1	≥16	Designated for platform use
    // 0	0	Instruction address misaligned	
    // 0	1	Instruction access fault	
    // 0	2	Illegal instruction	
    // 0	3	Breakpoint	
    // 0	4	Load address misaligned	
    // 0	5	Load access fault	
    // 0	6	Store/AMO address misaligned	
    // 0	7	Store/AMO access fault	
    // 0	8	Environment call from U-mode	
    // 0	9	Environment call from S-mode	
    // 0	10	Reserved	
    // 0	11	Environment call from M-mode	
    // 0	12	Instruction page fault	
    // 0	13	Load page fault	
    // 0	14	Reserved	
    // 0	15	Store/AMO page fault	
    // 0	16–23	Reserved	
    // 0	24–31	Designated for custom use	
    // 0	32–47	Reserved	
    // 0	48–63	Designated for custom use	
    // 0	≥64	Reserved
    //
    // Entry:
    // 1) Save PC to MEPC
    // 2) Save current privilege level to MSTATUS[MPP[1:0]]
    // 3) Save MIE[*] to MSTATUS[MPIE]
    // 4) Set MCAUSE[31] and MCAUSE[30:0] to 7 (interrupt, machine timer interrupt)
    // 4a) Set MTVAL to the specific device code if this is an external machine interrupt (i.e. UART? Keyboard? etc)
    // 5) Set zero to MSTATUS[MIE]
    // 6) Set PC to MTVEC (hardware)
    // TODO: Also set a task context pointer in MSCRATCH?
    //
    // Exit:
    // 1) Set current privilege level to MSTATUS[MPP[1:0]]
    // 2) Set MIE[*] to MSTATUS[MPIE]
    // 4) Set MEPC to PC (hardware)

    // NOTE: Hardware calls these instructions with the same PC
    // of the instruction after which the IRQ occurs
    // (i.e. PC+4 which is the return address)

    void __attribute__((naked, section (".LUT"))) enterTimerISR()
    {
        asm volatile(
            // 0
            "addi sp,sp,-4;"            // Save current A5 
            "sw a5, 0(sp);"
            // 1
            "auipc a5, 0;"              // Grab PC+0
            "csrrw a5, mepc, a5;"       // Set MEPC to current PC
            // 2
            ";"                         // No privileges to save in this architecture
            // 3
            "li a5, 128;"               // Generate mask for bit 7
            "csrrc a5, mie, a5;"        // Extract MIE[7(MTIE)] and set it to zero
            "csrrs a5, mstatus, a5;"    // Copy it to MSTATUS[7(MPIE)]
            // 4
            "li a5, 0x80000007;"        // Set MCAUSE[31] for interrupt and set MCAUSE[30:0] to 7
            "csrrw a5, mcause, a5;"
            // 5
            "li a5, 8;"                 // Generate mask for bit 3
            "csrrc a5, mstatus, a5;"    // Clear MSTATUS[3(MIE)]
            // 6
            //"csrr a5, mtvec;"           // Grab MTVEC
            //"jalr 0(a5);"               // Enter the ISR
        );
    }

    void __attribute__((naked, section (".LUT"))) leaveTimerISR()
    {
        asm volatile(
            // 1
            ";" // No privileges to restore on this architecture
            // 2
            "li a5, 128;"               // Generate mask for bit 7
            "csrrc a5, mstatus, a5;"    // Extract MSTATUS[7(MPIE)] and set it to zero
            "csrrs a5, mie, a5;"        // Copy it to MIE[7(MTIE)]
            // 3
            "li a5, 8;"                 // Generate mask for bit 3
            "csrrs a5, mstatus, a5;"    // Set MSTATUS[3(MIE)]
            // 4
            "lw a5, 0(sp);"             // Restore old A5 before ISR entry
            "addi sp,sp,4;"
            // 5
            // Hardware sets PC <= MEPC;
         );
    }

    void __attribute__((naked, section (".boot"))) _start()
    {
        enterTimerISR();
        leaveTimerISR();
    }

    void __attribute__((noreturn, naked, section (".boot"))) _exit(int x)
    {
        // NOOP
    }
};
