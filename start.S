    .section .text
    .globl _start
_start:
    li  t0, 0              # DMEM base
    li  t1, 0x11111111
    sw  t1, 0(t0)
    li  t1, 0x22222222
    sw  t1, 4(t0)
    li  t1, 1
    sw  t1, 8(t0)
1:  j   1b
