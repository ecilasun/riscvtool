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
    .text :
   {
      *(.text)
   }
   .sdata :
   {
      PROVIDE(__global_pointer$ = . + (4K/2));
      *(.sdata .sdata.*)
   }
   .rodata :
   {
      *(.rodata.*)
   }
   .got :
   {
      *(.got.*)
   }
   .bss :
   {
      *(.bss.*)
   }
   __heap_top = .;
}