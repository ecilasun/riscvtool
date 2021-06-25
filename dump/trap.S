trap_handler:

addi sp,sp,-144
sw	ra,140(sp)
sw	t0,136(sp)
sw	t1,132(sp)
sw	t2,128(sp)
sw	a0,124(sp)
sw	a1,120(sp)
sw	a2,116(sp)
sw	a3,112(sp)
sw	a4,108(sp)
sw	a5,104(sp)
sw	a6,100(sp)
sw	a7,96(sp)
sw	t3,92(sp)
sw	t4,88(sp)
sw	t5,84(sp)
sw	t6,80(sp)
fsw	ft0,76(sp)
fsw	ft1,72(sp)
fsw	ft2,68(sp)
fsw	ft3,64(sp)
fsw	ft4,60(sp)
fsw	ft5,56(sp)
fsw	ft6,52(sp)
fsw	ft7,48(sp)
fsw	fa0,44(sp)
fsw	fa1,40(sp)
fsw	fa2,36(sp)
fsw	fa3,32(sp)
fsw	fa4,28(sp)
fsw	fa5,24(sp)
fsw	fa6,20(sp)
fsw	fa7,16(sp)
fsw	ft8,12(sp)
fsw	ft9,8(sp)
fsw	ft10,4(sp)
fsw	ft11,0(sp)

csrr t0, mcause
li t1, 7
beq t0,t1, timerinterrupt
li t1, 3
beq t0,t1, breakpointinterrupt
li t1, 11
beq t0,t1, externalinterrupt

lw	ra,140(sp)
lw	t0,136(sp)
lw	t1,132(sp)
lw	t2,128(sp)
lw	a0,124(sp)
lw	a1,120(sp)
lw	a2,116(sp)
lw	a3,112(sp)
lw	a4,108(sp)
lw	a5,104(sp)
lw	a6,100(sp)
lw	a7,96(sp)
lw	t3,92(sp)
lw	t4,88(sp)
lw	t5,84(sp)
lw	t6,80(sp)
flw	ft0,76(sp)
flw	ft1,72(sp)
flw	ft2,68(sp)
flw	ft3,64(sp)
flw	ft4,60(sp)
flw	ft5,56(sp)
flw	ft6,52(sp)
flw	ft7,48(sp)
flw	fa0,44(sp)
flw	fa1,40(sp)
flw	fa2,36(sp)
flw	fa3,32(sp)
flw	fa4,28(sp)
flw	fa5,24(sp)
flw	fa6,20(sp)
flw	fa7,16(sp)
flw	ft8,12(sp)
flw	ft9,8(sp)
flw	ft10,4(sp)
flw	ft11,0(sp)
addi	sp,sp,144
mret


timerinterrupt:
ret

breakpointinterrupt:
ret

externalinterrupt:
ret