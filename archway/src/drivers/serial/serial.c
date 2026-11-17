/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file serial.c
 * @brief UART driver
 */

#include <drivers/serial/serial.h>
#include <archway/memory_layout.h>
#include <stdint.h>

#define UART_BASE 0x1fe001e0
#define UART_THR 0
#define UART_DLL 0
#define UART_IER 1
#define UART_DLM 1
#define UART_FCR 2
#define UART_LCR 3
#define UART_LSR 5
#define UART_LCR_8N1 0x03
#define UART_LCR_DLAB (1U << 7)
#define UART_FCR_ENABLE (1U << 0)
#define UART_FCR_CLEAR_RX (1U << 1)
#define UART_FCR_CLEAR_TX (1U << 2)
#define UART_LSR_THR_EMPTY (1U << 5)
#define UART_BAUD_DIVISOR_115200 1

static volatile uint8_t *const uart =
    (volatile uint8_t *)(uintptr_t)(ARCHWAY_MMIO_BASE + UART_BASE);

void
serial_init(void) {
    uart[UART_LCR] = UART_LCR_8N1;
    uart[UART_IER] = 0;
    uart[UART_LCR] = UART_LCR_DLAB | UART_LCR_8N1;
    uart[UART_DLL] = UART_BAUD_DIVISOR_115200;
    uart[UART_DLM] = 0;
    uart[UART_LCR] = UART_LCR_8N1;
    uart[UART_FCR] = UART_FCR_ENABLE | UART_FCR_CLEAR_RX | UART_FCR_CLEAR_TX;
}

void
serial_write_char(char character) {
    while ((uart[UART_LSR] & UART_LSR_THR_EMPTY) == 0) {
        __asm__ volatile("nop");
    }
    uart[UART_THR] = (uint8_t)character;
}

void
serial_write(const char *text) {
    while (*text != '\0') {
        if (*text == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*text++);
    }
}
