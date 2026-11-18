/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file serial.h
 * @brief Serial driver inf
 */

#ifndef ARCHWAY_SERIAL_H
#define ARCHWAY_SERIAL_H

#include <stddef.h>

void serial_init(void);
void serial_write_char(char character);
void serial_write(const char *text);
void serial_write_buffer(const char *buffer, size_t size);
void serial_write_emergency(const char *buffer, size_t size);

#endif
