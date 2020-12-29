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
   .rodata :
   {
      *(.rodata.*)
   }
}