// Use NekoYon GPU assembler to compile

// Prologue
_start:
jmp zero

// Test program
// Write 4 pixels to VRAM page 0
// Flip to page 1 to show result
_main:
vpage zero
setregi r4, 0xFFFFFFFF
setregi r5, 0x80000000
store.w r5, r4
setregi r8, 0x00000001
vpage r8

// Epilogue
_exit:
store.w zero, zero
setregi r1, 0x0000FFF8
store.w r1, zero
jmp zero
