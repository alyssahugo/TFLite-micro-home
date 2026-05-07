    .section .text
    .globl _start
_start:
    # Write a known value to a normal DMEM location (0x40)
    lui   t0, 0x0
    addi  t0, t0, 0x40        # t0 = 0x00000040
    li    t1, 0x12345678
    sw    t1, 0(t0)

    # Mailbox addresses used by your TB
    # SIG0: 0x000FFFF4  SIG1: 0x000FFFF8  DONE: 0x000FFFFC
    lui   t0, 0x00100         # t0 = 0x0010_0000
    addi  t0, t0, -12         # t0 = 0x000F_FFF4

    li    t1, 0x11111111
    sw    t1, 0(t0)           # SIG0

    li    t1, 0x22222222
    sw    t1, 4(t0)           # SIG1

    li    t1, 1
    sw    t1, 8(t0)           # DONE

halt:
    j halt
