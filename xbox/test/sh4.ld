/* the SH-4 harness's layout for qemu-sh4: no .stack section, since qemu
 * gives the program its stack */
ENTRY(_start)
/* one segment, writable and executable, as the console's RAM is: some of
 * the original assembly keeps its variables in its code section */
PHDRS
{
  all PT_LOAD FLAGS(7);
}
SECTIONS
{
  . = 0x10000000;
  .text : { *(.text .text.*) } :all
  .rodata : { *(.rodata .rodata.*) } :all
  .data : { *(.data .data.*) } :all
  .bss : { *(.bss .bss.* COMMON) } :all
  /DISCARD/ : { *(.stack) *(.comment) }
}
