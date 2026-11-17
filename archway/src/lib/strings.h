/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file strings.h
 * @brief libc memory functions.
 */

#ifndef ARCHWAY_STRINGS_H
#define ARCHWAY_STRINGS_H

#include <stddef.h>

int memcmp(const void *left, const void *right, size_t count);
void *memcpy(void *restrict destination, const void *restrict source, size_t count);
void *memmove(void *destination, const void *source, size_t count);
void *memset(void *destination, int value, size_t count);
int strcmp(const char *left, const char *right);
size_t strlen(const char *text);

#endif
