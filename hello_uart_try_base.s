.data
msg: .asciz "HELLO"

.text

_start:

addi a0, x0, 0      # address of msg
lui  t0, 0x40600    # UART base

loop:
lbu a1, 0(a0)
beq  a1, x0, done

wait_tx:
lw   a2, 8(t0)
andi a2, a2, 8
bne  a2, x0, wait_tx

sb   a1, 4(t0)

addi a0, a0, 1
jal  x0, loop

done:
jal x0, done
