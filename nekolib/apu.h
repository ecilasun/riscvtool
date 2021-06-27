
// APU command macros
#define APU22BITIMM(_immed_) (_immed_&0x003FFFFF)
#define APU10BITIMM(_immed_) ((_immed_&0xFFC00000)>>22)
#define APUOPCODE(_cmd_, _rs_, _rd_, _imm_) (_imm_<<10)|(_rd_<<7)|(_rs_<<4)|(_cmd_)
#define APUOPCODE2(_cmd_, _rs_, _rd_,_mask_, _imm_) (_imm_<<14) | (_mask_<<10) | (_rd_<<7) | (_rs_<<4) | (_cmd_);
#define APUOPCODE3(_cmd_, _rs1_, _rs2_, _rs3_, _imm_) (_imm_<<13)|(_rs3_<<10)|(_rs2_<<7)|(_rs1_<<4)|(_cmd_)

// APU opcodes (only lower 3 bits out of 4 used)

// Set register lower 22 bits when source register is zero, otherwise sets uppper 10 bits
#define APUSETREGISTER 0x1

// Write DWORD in rs onro SYSRAM address in rd, effectively signalling CPU from GPU side
#define APUAMEMOUT 0x6

inline void APUSetRegister(const uint8_t reg, uint32_t val) {
    *IO_APUCommandFIFO = GPUOPCODE(APUSETREGISTER, 0, reg, APU22BITIMM(val));
    *IO_APUCommandFIFO = GPUOPCODE(APUSETREGISTER, reg, reg, APU10BITIMM(val));
}

inline void GPUWriteToAudioMemory(const uint8_t countRegister, const uint8_t sysramPointerReg)
{
    *IO_APUCommandFIFO = APUOPCODE(APUAMEMOUT, countRegister, sysramPointerReg, 0);
}
