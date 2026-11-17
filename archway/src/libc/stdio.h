/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file stdio.h
 */

#ifndef ARCHWAY_LIBC_STDIO_H
#define ARCHWAY_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

int vsnprintf(char *restrict buffer, size_t size, const char *restrict format, va_list arguments);

#endif
