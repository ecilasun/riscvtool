MEMORY{
   RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 65536
}

SECTIONS
{
   __stack = ORIGIN(RAM) + LENGTH(RAM) - 4;
   .boot :
   {
      . = 0x0000000;
      *(.boot)
   }
   .bss :
   {
      *(.bss.*)
   }
    .text :
   {
      *(.text)
   }
   .data :
   {
      *(.data.*)
   }
   .rodata :
   {
      *(.rodata.*)
   }
   COMMON :
   {
      *(COMMON*)
   }
   .sdata :
   {
      PROVIDE(__global_pointer$ = . + (4K/2));
      *(.sdata .sdata.*)
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