/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file strings.c
 * @brief libc memory functions.
 */

#include <lib/strings.h>

#include <stdint.h>

int
memcmp(const void *left, const void *right, size_t count) {
    const unsigned char *a = left;
    const unsigned char *b = right;
    for (size_t i = 0; i < count; ++i) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

void *
memcpy(void *restrict destination, const void *restrict source, size_t count) {
    unsigned char *out = destination;
    const unsigned char *in = source;
    for (size_t i = 0; i < count; ++i) {
        out[i] = in[i];
    }
    return destination;
}

void *
memmove(void *destination, const void *source, size_t count) {
    unsigned char *out = destination;
    const unsigned char *in = source;
    if ((uintptr_t)out < (uintptr_t)in) {
        for (size_t i = 0; i < count; ++i) {
            out[i] = in[i];
        }
    } else if ((uintptr_t)out > (uintptr_t)in) {
        while (count != 0) {
            --count;
            out[count] = in[count];
        }
    }
    return destination;
}

void *
memset(void *destination, int value, size_t count) {
    unsigned char *out = destination;
    for (size_t i = 0; i < count; ++i) {
        out[i] = (unsigned char)value;
    }
    return destination;
}

int
strcmp(const char *left, const char *right) {
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return (unsigned char)*left - (unsigned char)*right;
}

size_t
strlen(const char *text) {
    const char *end = text;
    while (*end != '\0') {
        ++end;
    }
    return (size_t)(end - text);
}
