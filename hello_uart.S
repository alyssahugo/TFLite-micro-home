.text
start:
    lui t0, 0x40600        # t0 = 0x40600000
    addi a1, zero, 72      # a1 = 'H'
    sw a1, 4(t0)           # write to UART TX register

loop:
    jal zero, loop