.text
start:
    lui  t0, 0x40600
    addi a1, zero, 90
    sw   a1, 4(t0)
loop:
    jal  zero, loop