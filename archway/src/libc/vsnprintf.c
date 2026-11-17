/*
 * SPDX-License-Identifier: MulanPSL-2.0
 *
 * @file vsnprintf.c
 * @brief Bounded vsnprintf: integers, %s, %c, %p, %n. No floating-point or wide-char support.
 *
 * Returns the would-be length (excluding NUL), or -1 on an invalid format, unsupported
 * conversion, or a count above INT_MAX.
 * Nonempty buffer is always NUL-terminated.
 */

#include <libc/stdio.h>

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

enum length_modifier {
    LENGTH_DEFAULT,
    LENGTH_CHAR,
    LENGTH_SHORT,
    LENGTH_LONG,
    LENGTH_LONG_LONG,
    LENGTH_INTMAX,
    LENGTH_SIZE,
    LENGTH_PTRDIFF,
};

struct format_spec {
    bool left;
    bool plus;
    bool space;
    bool alternate;
    bool zero;
    bool width_from_arg;
    bool precision_from_arg;
    bool precision_overflow;
    int width;
    int precision;
    enum length_modifier length;
    char conversion;
};

struct output {
    char *buffer;
    size_t size;
    size_t count;
    bool failed;
};

/* abi compat checks */
_Static_assert(
    sizeof(size_t) == sizeof(ptrdiff_t), "size_t and ptrdiff_t must have matching sizes"
);
_Static_assert(
    SIZE_MAX >= UINT_MAX && PTRDIFF_MAX >= INT_MAX,
    "size_t and ptrdiff_t must not be narrower than int"
);

static bool
prepare_write(struct output *out, size_t length, size_t *writable) {
    if (out->failed) {
        return false;
    }
    if (length > (size_t)INT_MAX - out->count) {
        out->failed = true;
        return false;
    }
    *writable = 0;
    if (out->size != 0 && out->count < out->size - 1) {
        *writable = out->size - 1 - out->count;
        if (*writable > length) {
            *writable = length;
        }
    }
    out->count += length;
    return true;
}

static void
put_repeat(struct output *out, char character, size_t count) {
    size_t writable;
    if (!prepare_write(out, count, &writable)) {
        return;
    }
    for (size_t i = 0; i < writable; ++i) {
        out->buffer[out->count - count + i] = character;
    }
}

static void
put_char(struct output *out, char character) {
    put_repeat(out, character, 1);
}

static void
put_data(struct output *out, const char *data, size_t length) {
    size_t writable;
    if (!prepare_write(out, length, &writable)) {
        return;
    }
    for (size_t i = 0; i < writable; ++i) {
        out->buffer[out->count - length + i] = data[i];
    }
}

static size_t
bounded_length(const char *text, size_t maximum) {
    size_t length = 0;
    while (length < maximum && text[length] != '\0') {
        ++length;
    }
    return length;
}

static int
parse_decimal(const char **cursor, bool *overflow) {
    int value = 0;
    while (**cursor >= '0' && **cursor <= '9') {
        int digit = *(*cursor)++ - '0';
        if (value > (INT_MAX - digit) / 10) {
            *overflow = true;
        } else if (!*overflow) {
            value = value * 10 + digit;
        }
    }
    return value;
}

static bool
supported_conversion(char conversion, enum length_modifier length) {
    switch (conversion) {
    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'X': return true;
    case 's':
    case 'c':
    case 'p': return length == LENGTH_DEFAULT;
    case 'n': return true;
    default: return false;
    }
}

static bool
validate_spec(const struct format_spec *spec) {
    bool has_flags = spec->left || spec->plus || spec->space || spec->alternate || spec->zero;
    switch (spec->conversion) {
    case 's': return !spec->plus && !spec->space && !spec->alternate && !spec->zero;
    case 'c':
        return !spec->plus && !spec->space && !spec->alternate && !spec->zero && spec->precision < 0
               && !spec->precision_from_arg;
    case 'p':
        return !spec->plus && !spec->space && !spec->alternate && spec->precision < 0
               && !spec->precision_from_arg;
    case 'd':
    case 'i': return !spec->alternate;
    case 'u': return !spec->plus && !spec->space && !spec->alternate;
    case 'o': return !spec->plus && !spec->space;
    case 'x':
    case 'X': return !spec->plus && !spec->space;
    case 'n':
        return !has_flags && !spec->width_from_arg && spec->width == 0 && spec->precision < 0
               && !spec->precision_from_arg;
    default: return false;
    }
}

static bool
parse_spec(const char **cursor, struct format_spec *spec) {
    *spec = (struct format_spec){.precision = -1, .length = LENGTH_DEFAULT};

    bool parse_flags = true;
    while (parse_flags) {
        switch (**cursor) {
        case '-':
            spec->left = true;
            ++*cursor;
            break;
        case '+':
            spec->plus = true;
            ++*cursor;
            break;
        case ' ':
            spec->space = true;
            ++*cursor;
            break;
        case '#':
            spec->alternate = true;
            ++*cursor;
            break;
        case '0':
            spec->zero = true;
            ++*cursor;
            break;
        default: parse_flags = false; break;
        }
    }

    bool width_overflow = false;
    if (**cursor == '*') {
        spec->width_from_arg = true;
        ++*cursor;
    } else {
        spec->width = parse_decimal(cursor, &width_overflow);
    }

    if (**cursor == '.') {
        ++*cursor;
        spec->precision = 0;
        if (**cursor == '*') {
            spec->precision_from_arg = true;
            ++*cursor;
        } else {
            spec->precision = parse_decimal(cursor, &spec->precision_overflow);
            if (spec->precision_overflow) {
                spec->precision = INT_MAX;
            }
        }
    }

    if (**cursor == 'h') {
        ++*cursor;
        spec->length = **cursor == 'h' ? (++*cursor, LENGTH_CHAR) : LENGTH_SHORT;
    } else if (**cursor == 'l') {
        ++*cursor;
        spec->length = **cursor == 'l' ? (++*cursor, LENGTH_LONG_LONG) : LENGTH_LONG;
    } else if (**cursor == 'j') {
        ++*cursor;
        spec->length = LENGTH_INTMAX;
    } else if (**cursor == 'z') {
        ++*cursor;
        spec->length = LENGTH_SIZE;
    } else if (**cursor == 't') {
        ++*cursor;
        spec->length = LENGTH_PTRDIFF;
    }

    spec->conversion = **cursor;
    if (spec->conversion == '\0' || !supported_conversion(spec->conversion, spec->length)
        || width_overflow) {
        return false;
    }

    ++*cursor;
    if (!validate_spec(spec)) {
        return false;
    }
    return !spec->precision_overflow || spec->conversion == 's';
}

struct integer_layout {
    char digits[sizeof(uintmax_t) * CHAR_BIT];
    size_t digit_count;
    char prefix[3];
    size_t prefix_size;
    size_t zeroes;
    size_t padding;
    bool zero_padding;
    bool left;
};

static struct integer_layout
compute_integer_layout(uintmax_t value, bool negative, const struct format_spec *spec) {
    struct integer_layout layout = {0};
    int precision = spec->conversion == 'p' ? -1 : spec->precision;
    unsigned base;
    switch (spec->conversion) {
    case 'o': base = 8; break;
    case 'u':
    case 'd':
    case 'i': base = 10; break;
    default: base = 16; break;
    }
    bool uppercase = spec->conversion == 'X';
    bool nonzero = value != 0;
    const char *alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    if (nonzero || precision != 0) {
        do {
            layout.digits[sizeof(layout.digits) - ++layout.digit_count] = alphabet[value % base];
            value /= base;
        } while (value != 0);
    }

    layout.zeroes = precision > 0 && (size_t)precision > layout.digit_count
                        ? (size_t)precision - layout.digit_count
                        : 0;
    bool octal_prefix = spec->alternate && base == 8
                        && (layout.digit_count == 0
                            || layout.digits[sizeof(layout.digits) - layout.digit_count] != '0')
                        && layout.zeroes == 0;
    bool hex_prefix = base == 16 && (spec->conversion == 'p' || (spec->alternate && nonzero));
    bool signed_conversion = spec->conversion == 'd' || spec->conversion == 'i';
    char sign = negative                           ? '-'
                : signed_conversion && spec->plus  ? '+'
                : signed_conversion && spec->space ? ' '
                                                   : '\0';
    if (sign != '\0') {
        layout.prefix[layout.prefix_size++] = sign;
    }
    if (hex_prefix) {
        layout.prefix[layout.prefix_size++] = '0';
        layout.prefix[layout.prefix_size++] = uppercase ? 'X' : 'x';
    }
    if (octal_prefix) {
        layout.prefix[layout.prefix_size++] = '0';
    }
    size_t content_size = layout.prefix_size + layout.zeroes + layout.digit_count;
    layout.padding = spec->width > 0 && (size_t)spec->width > content_size
                         ? (size_t)spec->width - content_size
                         : 0;
    layout.zero_padding = spec->zero && precision < 0;
    layout.left = spec->left;
    return layout;
}

static void
emit_integer(struct output *out, const struct integer_layout *layout) {
    if (!layout->left && !layout->zero_padding) {
        put_repeat(out, ' ', layout->padding);
    }
    put_data(out, layout->prefix, layout->prefix_size);
    if (!layout->left && layout->zero_padding) {
        put_repeat(out, '0', layout->padding);
    }
    put_repeat(out, '0', layout->zeroes);
    put_data(
        out, layout->digits + sizeof(layout->digits) - layout->digit_count, layout->digit_count
    );
    if (layout->left) {
        put_repeat(out, ' ', layout->padding);
    }
}

static void
format_integer(struct output *out, uintmax_t value, bool negative, const struct format_spec *spec) {
    struct integer_layout layout = compute_integer_layout(value, negative, spec);
    emit_integer(out, &layout);
}

static void
format_string(struct output *out, const char *text, const struct format_spec *spec) {
    if (text == NULL) {
        text = "(null)";
    }
    size_t length = spec->precision >= 0 ? bounded_length(text, (size_t)spec->precision)
                                         : bounded_length(text, SIZE_MAX);
    if (spec->precision_overflow && length == (size_t)INT_MAX && text[length] != '\0') {
        out->failed = true;
        return;
    }
    size_t padding =
        spec->width > 0 && (size_t)spec->width > length ? (size_t)spec->width - length : 0;
    if (!spec->left) {
        put_repeat(out, ' ', padding);
    }
    put_data(out, text, length);
    if (spec->left) {
        put_repeat(out, ' ', padding);
    }
}

static void
format_char(struct output *out, int argument, const struct format_spec *spec) {
    size_t padding = spec->width > 1 ? (size_t)spec->width - 1 : 0;
    if (!spec->left) {
        put_repeat(out, ' ', padding);
    }
    put_char(out, (char)(unsigned char)argument);
    if (spec->left) {
        put_repeat(out, ' ', padding);
    }
}

static void
store_count(struct output *out, enum length_modifier length, va_list *arguments) {
    switch (length) {
    case LENGTH_CHAR: *va_arg(*arguments, signed char *) = (signed char)out->count; break;
    case LENGTH_SHORT: *va_arg(*arguments, short *) = (short)out->count; break;
    case LENGTH_LONG: *va_arg(*arguments, long *) = (long)out->count; break;
    case LENGTH_LONG_LONG: *va_arg(*arguments, long long *) = (long long)out->count; break;
    case LENGTH_INTMAX: *va_arg(*arguments, intmax_t *) = (intmax_t)out->count; break;
    case LENGTH_SIZE:
    case LENGTH_PTRDIFF: *va_arg(*arguments, ptrdiff_t *) = (ptrdiff_t)out->count; break;
    default: *va_arg(*arguments, int *) = (int)out->count; break;
    }
}

static intmax_t
read_signed(va_list *arguments, enum length_modifier length) {
    switch (length) {
    case LENGTH_CHAR: return (signed char)va_arg(*arguments, int);
    case LENGTH_SHORT: return (short)va_arg(*arguments, int);
    case LENGTH_LONG: return va_arg(*arguments, long);
    case LENGTH_LONG_LONG: return va_arg(*arguments, long long);
    case LENGTH_INTMAX: return va_arg(*arguments, intmax_t);
    case LENGTH_SIZE:
    case LENGTH_PTRDIFF: return va_arg(*arguments, ptrdiff_t);
    default: return va_arg(*arguments, int);
    }
}

static uintmax_t
read_unsigned(va_list *arguments, enum length_modifier length) {
    switch (length) {
    case LENGTH_CHAR: return (unsigned char)va_arg(*arguments, int);
    case LENGTH_SHORT: return (unsigned short)va_arg(*arguments, int);
    case LENGTH_LONG: return va_arg(*arguments, unsigned long);
    case LENGTH_LONG_LONG: return va_arg(*arguments, unsigned long long);
    case LENGTH_INTMAX: return va_arg(*arguments, uintmax_t);
    case LENGTH_SIZE:
    case LENGTH_PTRDIFF: return va_arg(*arguments, size_t);
    default: return va_arg(*arguments, unsigned int);
    }
}

static bool
resolve_star_args(struct format_spec *spec, va_list *args) {
    if (spec->width_from_arg) {
        spec->width = va_arg(*args, int);
        if (spec->width < 0) {
            if (spec->width == INT_MIN) {
                return false;
            }
            spec->left = true;
            spec->width = -spec->width;
        }
    }
    if (spec->precision_from_arg) {
        spec->precision = va_arg(*args, int);
        if (spec->precision < 0) {
            spec->precision = -1;
        }
    }

    return true;
}

static void
format_signed(struct output *out, intmax_t number, const struct format_spec *spec) {
    bool negative = number < 0;
    uintmax_t magnitude = negative ? (uintmax_t)(-(number + 1)) + 1 : (uintmax_t)number;
    format_integer(out, magnitude, negative, spec);
}

static void
terminate(struct output *out) {
    if (out->size != 0) {
        size_t index = out->count < out->size ? out->count : out->size - 1;
        out->buffer[index] = '\0';
    }
}

int
vsnprintf(char *restrict buffer, size_t size, const char *restrict format, va_list arguments) {
    if (buffer == NULL && size != 0) {
        return -1;
    }
    if (format == NULL) {
        if (size != 0) {
            buffer[0] = '\0';
        }
        return -1;
    }

    struct output out = {.buffer = buffer, .size = size, .count = 0, .failed = false};
    va_list args;
    va_copy(args, arguments);
    const char *cursor = format;
    while (*cursor != '\0' && !out.failed) {
        if (*cursor != '%') {
            const char *start = cursor;
            while (*cursor != '\0' && *cursor != '%') {
                ++cursor;
            }
            put_data(&out, start, (size_t)(cursor - start));
            continue;
        }
        ++cursor;
        if (*cursor == '%') {
            put_char(&out, *cursor++);
            continue;
        }

        struct format_spec spec;
        if (!parse_spec(&cursor, &spec)) {
            out.failed = true;
            break;
        }

        if (!resolve_star_args(&spec, &args)) {
            out.failed = true;
            break;
        }
        switch (spec.conversion) {
        case 's': format_string(&out, va_arg(args, void *), &spec); break;
        case 'c': format_char(&out, va_arg(args, int), &spec); break;
        case 'p': format_integer(&out, (uintptr_t)va_arg(args, void *), false, &spec); break;
        case 'n': store_count(&out, spec.length, &args); break;
        case 'd':
        case 'i': format_signed(&out, read_signed(&args, spec.length), &spec); break;
        case 'u':
        case 'o':
        case 'x':
        case 'X': format_integer(&out, read_unsigned(&args, spec.length), false, &spec); break;
        /* supported_conversion rejects all other conversions. */
        default: out.failed = true; break;
        }
    }

    terminate(&out);
    va_end(args);
    return out.failed ? -1 : (int)out.count;
}
