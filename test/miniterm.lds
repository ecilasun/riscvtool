MEMORY{
   RAM (rwx) : ORIGIN = 0x000002C0, LENGTH = 65536
}

SECTIONS
{
   __stack = ORIGIN(RAM) + LENGTH(RAM) - 16;
   .boot :
   {
      . = 0x00002C0;
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