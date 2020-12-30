SECTIONS
{
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
}