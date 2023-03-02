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
    // Entry:
    // 1) Save PC to MEPC
    // 2) Save current privilege level to MSTATUS[MPP[1:0]]
    // 3) Save MIE[*] to MSTATUS[MPIE]
    // 4) Set zero to MSTATUS[MIE]
    // 5) Set PC to MTVEC
    //
    // Exit:
    // 1) Set current privilege level to MSTATUS[MPP[1:0]]
    // 2) Set MIE[*] to MSTATUS[MPIE]
    // 4) Set MEPC to PC

    void __attribute__((naked, section (".LUT"))) enterTimerISR()
    {
        // NOTE: Hardware calls these instructions with the same PC
        // of the instruction after which the IRQ occurs
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
            "li a5, 8;"                 // Generate mask for bit 3
            "csrrc a5, mie, a5;"        // Clear MSTATUS[3(MIE)]
            // 5
            "csrr a5, mtvec;"           // Grab MTVEC
            "jalr 0(a5);"               // Enter the ISR
        );
    }

    void __attribute__((naked, section (".LUT"))) leaveTimerISR()
    {
        asm volatile(
            // 1
            ";" // No privileges to restore
            // 2
            "li a5, 128;"               // Generate mask for bit 7
            "csrrc a5, mstatus, a5;"    // Extract MSTATUS[7(MPIE)] and set it to zero
            "csrrs a5, mstatus, a5;"    // Copy it to MIE[7(MTIE)]
            // 3
            "li a5, 8;"                 // Generate mask for bit 3
            "csrrs a5, mie, a5;"        // Set MSTATUS[3(MIE)]
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


/*
csrr	a5,mcause    csrrw	a5,mtvec,a5    csrrwi	a5,mstatus,8   csrrw	a4,0x801,a4 & csrrw	a0,0x800,a0
ENTERISR: begin
    // mie[11] -> hw, mie[7] -> timer, mie[3] -> software
    hwint <= hwint & {mie[11], mie[11]};
    trq <= trq & mie[7];
    illegalinstruction <= illegalinstruction & mie[3];
    ebreak <= ebreak & mie[3];
    ecall <= ecall & mie[3];
    execmode <= SAVEMIP;
end

SAVEMIP: begin
    csrwe <= 1'b1;
    csrop <= 3'b100; // Custom - CSR = CSREXT
    csrwenforce <= 1'b1;
    csrenforceindex <= `CSR_MIP;
    // Set corresponding interrupt pending bits
    // TODO: If we have a way to just send the | mask part we can get rid of 'mip' 
    csrext <= mip | {
        20'd0,
        {|hwint}, 3'b000,
        trq, 3'b000,
        (illegalinstruction || ebreak || ecall), 3'b000};
    execmode <= SAVEMEPC;
end

SAVEMEPC: begin
    csrwe <= 1'b1;
    csrop <= 3'b100; // Custom - CSR = CSREXT
    csrwenforce <= 1'b1;
    csrenforceindex <= `CSR_MEPC;
    csrext <= ebreak ? PC : adjacentPC;
    execmode <= SAVECAUSE;
end

SAVECAUSE: begin
    csrwe <= 1'b1;
    csrop <= 3'b100; // Custom - CSR = CSREXT
    csrwenforce <= 1'b1;
    csrenforceindex <= `CSR_MCAUSE;
    case (1'b1)
        // Priority is for hadrware interrupts
        (|hwint):				csrext <= 32'h8000000b; // [31]=1'b1(interrupt), 11->h/w
        trq:					csrext <= 32'h80000007; // [31]=1'b1(interrupt), 7->timer
        // Next, we handle the software exceptions
        illegalinstruction:		csrext <= 32'h00000002; // Illegal instruction
        ebreak:					csrext <= 32'h00000003; // Breakpoint
        //loadfault:			csrext <= 32'h00000005; // Load access fault
        //storefault:			csrext <= 32'h00000007; // Store/AMO access fault
        ecall:					csrext <= 32'h0000000b; // Environment call from machine mode
        default:				csrext <= 32'h00000000; // Reserved
    endcase
    execmode <= SAVEVAL;
end

SAVEVAL: begin
    csrwe <= 1'b1;
    csrop <= 3'b100; // Custom - CSR = CSREXT
    csrwenforce <= 1'b1;
    csrenforceindex <= `CSR_MTVAL;
    csrext <= 0;
    case (1'b1)
        // Interrupts
        (|hwint):			csrext <= {27'd0, hwint};
        trq:				csrext <= 32'd0;
        // Exceptions
        illegalinstruction,
        ebreak,
        //loadfault,
        //storefault,
        ecall:				csrext <= PC;
        default:			csrext <= 32'h00000000; // Reserved
    endcase
    // Done saving state, signal branch to interrupt vector
    // This will also let the fetch unit see our writes to mip register
    // so it won't trigger another interrupt on top of pending ones
    branchena <= 1'b1;
    branchPC <= mtvec;
    execmode <= FETCH;
    ebreak <= 1'b0; // Do not trigger ebreak anymore
end*/