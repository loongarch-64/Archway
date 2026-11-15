/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file strhlp.h
 * @brief String formatting helpers.
 */

#ifndef ARCHWAY_STRHLP_H
#define ARCHWAY_STRHLP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool append_ascii(uint16_t *buffer, size_t capacity, size_t *length, const char *text); // append ascii text to utf-16 buffer
bool append_hex(uint16_t *buffer, size_t capacity, size_t *length, uint64_t value, size_t digits);
bool append_decimal(uint16_t *buffer, size_t capacity, size_t *length, uint64_t value);

#endif
