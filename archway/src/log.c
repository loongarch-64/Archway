/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file log.c
 * @brief Just a printf.
 */

#include <drivers/serial/serial.h>
#include <libc/stdio.h>
#include <log.h>
#include <stdarg.h>
#include <stdbool.h>

static int
print_to_serial(const char *format, va_list arguments, bool exception) {
    char buffer[512];
    int written = vsnprintf(buffer, sizeof(buffer), format, arguments);
    if (written < 0) {
        return written;
    }
    size_t output_size = (size_t)written;
    if (output_size >= sizeof(buffer)) {
        output_size = sizeof(buffer) - 1;
    }
    if (exception) {
        serial_write_emergency(buffer, output_size);
    } else {
        serial_write_buffer(buffer, output_size);
    }
    return (int)output_size;
}

int
kprintf(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int written = print_to_serial(format, arguments, false);
    va_end(arguments);
    return written;
}

int
kprintf_exception(const char *format, ...) {
    va_list arguments;
    va_start(arguments, format);
    int written = print_to_serial(format, arguments, true);
    va_end(arguments);
    return written;
}
