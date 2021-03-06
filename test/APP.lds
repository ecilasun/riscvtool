MEMORY{
   RAM (rwx) : ORIGIN = 0x00000500, LENGTH = 65536
}

SECTIONS
{
    .text :
   {
      *(.text*)
      *(.rodata*)
      _etext = .;
   }
   .bss :
   {
      _sbss = . ;
      *(.bss*)
      *(COMMON*)
      _ebss = . ;
   }
   .data :
   {
      _sdata = .;
      *(.data*)
      _edata = .;
   }
   .sdata :
   {
      PROVIDE(__global_pointer$ = . + (4K/2));
      *(.sdata .sdata.*)
   }
   .stack (NOLOAD) : ALIGN(16)
   {
      . = . + 4096;
      . = ALIGN(16);
   } > RAM
   _end = . ;
}