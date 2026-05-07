#include <stdint.h>

extern "C" void DebugLog(const char* s) {
  volatile uint32_t* const UART_TX_FIFO = (volatile uint32_t*)(0x40600000 + 0x04);
  volatile uint32_t* const UART_STATUS  = (volatile uint32_t*)(0x40600000 + 0x08);

  if (s == nullptr) return;

  while (*s) {
    // Wait while TX FIFO is full.
    while ((*UART_STATUS) & 0x08) {
    }

    *UART_TX_FIFO = (uint32_t)(uint8_t)(*s);

    // Optional CR before LF for terminal friendliness.
    if (*s == '\n') {
      while ((*UART_STATUS) & 0x08) {
      }
      *UART_TX_FIFO = (uint32_t)'\r';
    }

    ++s;
  }
}