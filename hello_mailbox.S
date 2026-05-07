    .option norvc
    .section .text
    .globl _start
_start:
    lui   t0, 0x0
    addi  t0, t0, 0x40
    lui   t1, 0x12345
    addi  t1, t1, 0x678
    sw    t1, 0(t0)

    lui   t0, 0x00100
    addi  t0, t0, -12        # 0x000FFFF4

    lui   t1, 0x11111
    addi  t1, t1, 0x111
    sw    t1, 0(t0)

    lui   t1, 0x22222
    addi  t1, t1, 0x222
    sw    t1, 4(t0)

    addi  t1, x0, 1
    sw    t1, 8(t0)

halt:
    jal   x0, halt