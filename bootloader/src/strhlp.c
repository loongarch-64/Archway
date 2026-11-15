/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file strhlp.c
 * @brief String formatting helpers.
 */

#include "strhlp.h"

bool
append_ascii(uint16_t *buffer, size_t capacity, size_t *length, const char *text) {
    if (buffer == NULL || length == NULL || text == NULL || *length >= capacity) {
        return false;
    }

    while (*text != '\0') {
        if (*length + 1 >= capacity) {
            buffer[*length] = 0;
            return false;
        }
        buffer[(*length)++] = (uint8_t)*text++;
    }

    buffer[*length] = 0;
    return true;
}

bool
append_hex(
    uint16_t *buffer,
    size_t capacity,
    size_t *length,
    uint64_t value,
    size_t digits
) {
    static const char hex_digits[] = "0123456789abcdef";

    if (buffer == NULL || length == NULL || *length >= capacity || digits > 16
        || digits == 0 || digits > capacity - *length - 1
        || (digits < 16 && (value >> (digits * 4)) != 0)) {
        return false;
    }

    for (size_t i = digits; i > 0; --i) {
        size_t shift = (i - 1) * 4;
        buffer[(*length)++] = (uint8_t)hex_digits[(value >> shift) & 0xf];
    }

    buffer[*length] = 0;
    return true;
}

bool
append_decimal(
    uint16_t *buffer,
    size_t capacity,
    size_t *length,
    uint64_t value
) {
    uint16_t reversed[20];
    size_t count = 0;

    if (buffer == NULL || length == NULL || *length >= capacity) {
        return false;
    }

    do {
        reversed[count++] = (uint16_t)('0' + (value % 10));
        value /= 10;
    } while (value != 0);

    if (count > capacity - *length - 1) {
        buffer[*length] = 0;
        return false;
    }

    while (count > 0) {
        buffer[(*length)++] = reversed[--count];
    }

    buffer[*length] = 0;
    return true;
}
