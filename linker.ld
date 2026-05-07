OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
  INSTMEM (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  DDR     (rwx) : ORIGIN = 0x80000000, LENGTH = 64M
}

__stack_size = 64K;
__heap_size  = 64K;

SECTIONS
{
  . = ORIGIN(INSTMEM);

  .init : ALIGN(4)
  {
    KEEP(*(.init))
    KEEP(*(.text.startup))
    KEEP(*(.text.startup.*))
  } > INSTMEM

  .text : ALIGN(4)
  {
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
  } > INSTMEM

  . = ALIGN(16);

  .rodata : ALIGN(16)
  {
    *(.srodata)
    *(.srodata.*)
    *(.rodata)
    *(.rodata.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.r.*)
  } > DDR

  .data : ALIGN(16)
  {
    __data_start = .;
    *(.sdata)
    *(.sdata.*)
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d.*)
    __data_end = .;
    _edata = .;
  } > DDR

  .gcc_except_table : ALIGN(4)
  {
    *(.gcc_except_table)
    *(.gcc_except_table.*)
  } > DDR

  . = ALIGN(16);
  __global_pointer$ = . + 0x800;

  .bss (NOLOAD) : ALIGN(16)
  {
    __bss_start = .;
    *(.sbss)
    *(.sbss.*)
    *(.bss)
    *(.bss.*)
    *(COMMON)
    __bss_end = .;
  } > DDR

  .noinit (NOLOAD) : ALIGN(16)
  {
    *(.noinit)
    *(.noinit.*)
  } > DDR

  .heap (NOLOAD) : ALIGN(16)
  {
    __heap_start = .;
    . = . + __heap_size;
    __heap_end = .;
  } > DDR

  .stack (NOLOAD) : ALIGN(16)
  {
    __stack_bottom = .;
    . = . + __stack_size;
    __stack_top = .;
  } > DDR

  __end = .;
  _end = .;
  end = .;

  /DISCARD/ :
  {
    *(.comment)
    *(.note)
    *(.note.*)
    *(.eh_frame)
    *(.eh_frame.*)
    *(.riscv.attributes)
  }
}