MEMORY{
   RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 131072
}

SECTIONS
{
   .boot :
   {
      /* Reset vector */
      . = 0x00000100;
      *(.boot)
      _boot = .;
   }
    .text :
   {
      *(.text*)
      *(.rodata*)
      _etext = .;
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
   .bss :
   {
      _sbss = . ;
      *(.bss*)
      *(COMMON*)
      _ebss = . ;
   }
   .shstrtab :
   {
      *(.shstrtab)
   }
   .got :
   {
      *(.got.*)
   }
   _end = . ;
   __stack = ORIGIN(RAM) + LENGTH(RAM) - 16;
}