OUTPUT_ARCH(riscv)
ENTRY(_start)

/* Your core sees both memories starting at 0 (separate physical) */
MEMORY {
  IMEM (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  DMEM (rwx) : ORIGIN = 0x00000000, LENGTH = 2M

  /* A fake "load region" so sections don't overlap inside the ELF file.
     Choose something that doesn't overlap IMEM contents in the file. */
  DMEM_LMA (r) : ORIGIN = 0x00100000, LENGTH = 2M
}

SECTIONS {
  /* ---------- code in IMEM ---------- */
  .text : ALIGN(4) {
    KEEP(*(.init))
    KEEP(*(.init.*))
    *(.text*)
  } > IMEM

  /* ---------- rodata/data/bss live in DMEM at VMA 0, but are stored in ELF at DMEM_LMA ---------- */
  .rodata : ALIGN(4) {
    *(.rodata*)
    *(.sdata2*)
  } > DMEM AT> DMEM_LMA

  .data : ALIGN(4) {
    __data_start = .;
    *(.data*)
    *(.sdata*)
    __data_end = .;
  } > DMEM AT> DMEM_LMA

  .bss (NOLOAD) : ALIGN(4) {
    __bss_start = .;
    *(.sbss*)
    *(.bss*)
    *(COMMON)
    __bss_end = .;
  } > DMEM

  /* Useful symbols */
  _edata = __data_end;
  _end   = __bss_end;

  PROVIDE(__global_pointer$ = ORIGIN(DMEM) + 0x800);

  /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.note*) }
}