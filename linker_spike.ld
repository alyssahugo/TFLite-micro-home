OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
  RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 64M
}

SECTIONS
{
  . = ORIGIN(RAM);

  .text : ALIGN(4)
  {
    KEEP(*(.init))
    KEEP(*(.init.*))
    *(.text*)
    *(.rodata.text*)
    . = ALIGN(4);
  } > RAM

  .rodata : ALIGN(16)
  {
    *(.rodata*)
    *(.srodata*)
    *(.srodata.*)
    . = ALIGN(16);
  } > RAM

  .data : ALIGN(16)
  {
    __data_start = .;
    *(.data*)
    *(.sdata*)
    *(.sdata.*)
    . = ALIGN(16);
    __data_end = .;
  } > RAM

  .bss (NOLOAD) : ALIGN(16)
  {
    __bss_start = .;
    *(.sbss*)
    *(.bss*)
    *(COMMON)
    . = ALIGN(16);
    __bss_end = .;
  } > RAM

  .arena (NOLOAD) : ALIGN(16)
  {
    __arena_start = .;
    *(.arena)
    *(.arena*)
    . = ALIGN(16);
    __arena_end = .;
  } > RAM

  .stack (NOLOAD) : ALIGN(16)
  {
    . += 0x00008000;   /* 32 KB stack */
    . = ALIGN(16);
    __stack_top = .;
  } > RAM

  _end = .;

  PROVIDE(__global_pointer$ = __data_start + 0x800);

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