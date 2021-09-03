// Use NekoYon GPU assembler to compile

// Prologue
_start:
jmp zero                    // Halt until CPU overwrites with noop

_main:
vpage zero                  // Writes go to V-RAM page 0 (and flip)
setregi r15, 0x00000004     // Byte offset for second V-RAM address
setregi r4, 0xFFFFFFFF      // First set of 4 pixels
setregi r5, 0x80000000      // Initial V-RAM address
store.w r5, r4              // Write to V-RAM
setregi r4, 0x04050607      // Second set of 4 pixels
add.w r5, r15, r5           // Increment pointer
store.w r5, r4              // Write to V-RAM
setregi r8, 0x00000001      // Writes go to V-RAM page 1
vpage r8                    // Flip

// Epilogue
_exit:
store.w zero, zero          // Write HALT at G-RAM address 0x0000
setregi r1, 0x0000FFF8      // Set mailbox address
store.w r1, zero            // Write 'done' (0x0) to mailbox
jmp zero                    // Branch to top of G-RAM and halt
