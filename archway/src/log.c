/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file log.c
 * @brief Just a printf.
 */

#include <log.h>
#include <drivers/serial/serial.h>
#include <libc/stdio.h>

#include <stdarg.h>

int
kprintf(const char *format, ...) {
    char buffer[512];
    va_list arguments;
    va_start(arguments, format);
    int written = vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    if (written < 0) {
        return written;
    }
    size_t output_size = (size_t)written;
    if (output_size >= sizeof(buffer)) {
        output_size = sizeof(buffer) - 1;
    }
    for (size_t i = 0; i < output_size; ++i) {
        if (buffer[i] == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(buffer[i]);
    }
    return (int)output_size;
}
