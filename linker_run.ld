ENTRY(_start)

MEMORY
{
  IMEM (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  DDR  (rwx) : ORIGIN = 0x80000000, LENGTH = 16M
}

SECTIONS
{
  .text : ALIGN(4)
  {
    KEEP(*(.init))
    *(.text*)
    *(.rodata.text*)
    . = ALIGN(4);
  } > IMEM

  .rodata : ALIGN(16)
  {
    *(.rodata*)
    *(.srodata*)
    . = ALIGN(16);
  } > DDR

  .data : ALIGN(16)
  {
    __data_start = .;
    *(.data*)
    *(.sdata*)
    . = ALIGN(16);
    __data_end = .;
  } > DDR

  .bss (NOLOAD) : ALIGN(16)
  {
    __bss_start = .;
    *(.sbss*)
    *(.bss*)
    *(COMMON)
    . = ALIGN(16);
    __bss_end = .;
  } > DDR

  .arena (NOLOAD) : ALIGN(16)
  {
    __arena_start = .;
    *(.arena)
    *(.arena*)
    . = ALIGN(16);
    __arena_end = .;
  } > DDR

  .stack (NOLOAD) : ALIGN(16)
  {
    . += 0x00008000;   /* 32 KB stack */
    . = ALIGN(16);
    __stack_top = .;
  } > DDR

  /DISCARD/ :
  {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
    *(.eh_frame_hdr*)
    *(.riscv.attributes)
    *(.gnu*)
  }
}