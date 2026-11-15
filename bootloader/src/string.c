/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file string.c
 * @brief Memory functions.
 */

#include "string.h"
#include <stdint.h>

void *
memcpy(void *restrict destination, const void *restrict source, size_t size) {
    unsigned char *out = destination;
    const unsigned char *in = source;

    for (size_t i = 0; i < size; ++i) {
        out[i] = in[i];
    }
    return destination;
}

void *
memmove(void *destination, const void *source, size_t size) {
    unsigned char *out = destination;
    const unsigned char *in = source;
    uintptr_t out_address = (uintptr_t)destination;
    uintptr_t in_address = (uintptr_t)source;

    if (out_address > in_address && out_address - in_address < size) {
        while (size != 0) {
            --size;
            out[size] = in[size];
        }
    } else {
        for (size_t i = 0; i < size; ++i) {
            out[i] = in[i];
        }
    }
    return destination;
}

void *
memset(void *destination, int value, size_t size) {
    unsigned char *out = destination;

    for (size_t i = 0; i < size; ++i) {
        out[i] = (unsigned char)value;
    }
    return destination;
}

int
memcmp(const void *left, const void *right, size_t size) {
    const unsigned char *a = left;
    const unsigned char *b = right;

    for (size_t i = 0; i < size; ++i) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }
    return 0;
}
