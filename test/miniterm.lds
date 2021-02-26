MEMORY{
   RAM (rwx) : ORIGIN = 0x00000300, LENGTH = 65536
}

SECTIONS
{
   __stack = ORIGIN(RAM) + LENGTH(RAM) - 16;
   .boot :
   {
      . = 0x0000300;
      *(.boot)
   }
    .text :
   {
      *(.text)
   }
   .rodata :
   {
      *(.rodata.*)
   }
   .data :
   {
      *(.data.*)
   }
   .sdata :
   {
      PROVIDE(__global_pointer$ = . + (4K/2));
      *(.sdata .sdata.*)
   }
   .bss :
   {
      *(.bss.*)
   }
   COMMON :
   {
      *(COMMON*)
   }
   .shstrtab :
   {
      *(.shstrtab)
   }
   .got :
   {
      *(.got.*)
   }
}