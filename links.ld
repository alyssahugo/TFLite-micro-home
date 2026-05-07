ENTRY(_start)

MEMORY
{
    INSTMEM (rx)  : ORIGIN = 0x00000000, LENGTH = 16K
    DATAMEM (rwx) : ORIGIN = 0x80000000, LENGTH = 16K
}

SECTIONS
{
    .text : ALIGN(4)
    {
        *(.text.init)
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
    } > INSTMEM

    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data)
        *(.data.*)
        __data_end = .;
    } > DATAMEM

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    } > DATAMEM

    . = ALIGN(16);
    __stack_top = ORIGIN(DATAMEM) + LENGTH(DATAMEM);
}