MEMORY{
   RAM (rwx) : ORIGIN = 0x00000500, LENGTH = 65536
}
OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS
{
    .text :
   {
      *(.text*)
      *(.text.unlikely .text.*_unlikely .text.unlikely.*)
      *(.text.exit .text.exit.*)
      *(.text.startup .text.startup.*)
      *(.text.hot .text.hot.*)
      *(.stub .text.* .gnu.linkonce.t.*)
   }
   .init :
   {
     KEEP (*(SORT_NONE(.init)))
   }
   .plt            : { *(.plt) }
   .iplt           : { *(.iplt) }
   .fini           :
   {
      KEEP (*(SORT_NONE(.fini)))
   }
   PROVIDE (__etext = .);
   PROVIDE (_etext = .);
   PROVIDE (etext = .);
   .rodata :
   {
      *(.rodata .rodata.* .gnu.linkonce.r.*)
   }
   .rodata1 : { *(.rodata1) }
   .sdata2 :
   {
      *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
   }
   .eh_frame_hdr : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) }
   .sbss2          : { *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*) }
   .eh_frame       : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) }
   .gcc_except_table   : ONLY_IF_RO { *(.gcc_except_table
   .gcc_except_table.*) }
   .gnu_extab   : ONLY_IF_RO { *(.gnu_extab*) }
   .eh_frame       : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) }
   .gnu_extab      : ONLY_IF_RW { *(.gnu_extab) }
   .gcc_except_table   : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) }
   .exception_ranges   : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) }
   .tdata          : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
   .tbss           : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
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
   .preinit_array  :
   {
      PROVIDE_HIDDEN (__preinit_array_start = .);
      *(.preinit_array)
      PROVIDE_HIDDEN (__preinit_array_end = .);
   }
   .init_array     :
   {
      PROVIDE_HIDDEN (__init_array_start = .);
      *(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*))
      *(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors)
      PROVIDE_HIDDEN (__init_array_end = .);
   }
   .fini_array     :
   {
      PROVIDE_HIDDEN (__fini_array_start = .);
      *(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*))
      *(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors)
      PROVIDE_HIDDEN (__fini_array_end = .);
   }
   .ctors :
   {
      KEEP (*crtbegin.o(.ctors))
      KEEP (*crtbegin?.o(.ctors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
      KEEP (*(SORT(.ctors.*)))
      KEEP (*(.ctors))
   }
   .dtors :
   {
      KEEP (*crtbegin.o(.dtors))
      KEEP (*crtbegin?.o(.dtors))
      KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
      KEEP (*(SORT(.dtors.*)))
      KEEP (*(.dtors))
   }
   __bss_start = .;
   .sbss           :
   {
      *(.dynsbss)
      *(.sbss .sbss.* .gnu.linkonce.sb.*)
      *(.scommon)
   }
   .bss :
   {
      _sbss = . ;
      *(.bss*)
      *(COMMON*)
      _ebss = . ;
   }
   .stack (NOLOAD) : ALIGN(16)
   {
      . = . + 4096;
      . = ALIGN(16);
   } > RAM
   _end = . ;
}