    .section .init
    .globl _start
    .type _start, @function

_start:
    la sp, __stack_top

    # Zero .bss
    la a0, __bss_start
    la a1, __bss_end
1:
    bgeu a0, a1, 2f
    sw zero, 0(a0)
    addi a0, a0, 4
    j 1b
2:
    call main

3:
    j 3b