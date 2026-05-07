#include <stdint.h>

#define UART_BASE        0x40600000u
#define UART_TX_REG      (*(volatile uint32_t *)(UART_BASE + 0x04))

void _start(void) {
    UART_TX_REG = 'A';
    while (1) {}
}






