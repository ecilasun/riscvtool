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
   .eh_frame :
   {
      *(.eh_frame*)
   }
   .init_array :
   {
      *(.init_array*)
   }
   .fini_array :
   {
      *(.fini_array*)
   }
   .srodata :
   {
      *(.srodata*)
   }
   .data :
   {
      _sdata = .;
      *(.data*)
      _edata = .;
   }
   .got :
   {
      *(.got*)
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