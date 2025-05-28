#include "libqb-common.h"

#include "error_handle.h"
#include "qbs.h"
#include <cstdio>
#include <type_traits>

// Stack-allocating these seems uncomfortable
static char qbs_val_significant_digits[256];
static char qbs_val_built_number[256];

template <typename T> T qbs_val(qbs *s) {
    if (!s->len) {
        return 0;
    }

    union {
        uint64_t i;    // used when the input is an integer
        long double f; // used when the input is a floating-point number
    } value{};

    auto negate = false;
    auto num_significant_digits = 0;
    auto most_significant_digit_position = 0;
    auto exponent_value = 0;
    auto negate_exponent = false;
    auto num_exponent_digits = 0;
    auto step = 0; // 0: init, 1: int part, 2: frac part, 3: exp mark, 4: exp digits
    auto i = 0;
    char c;

    for (i = 0; i < s->len; i++) {
        c = (char)s->chr[i];

        switch (c) {
        case ' ':
        case '\t':
            continue; // skip whitespaces
            break;

        case '&':
            if (step == 0) {
                goto non_decimal;
            }
            goto finish;
            break;

        case '-':
            if (step == 0) {
                negate = true;
                step = 1;
            } else if (step == 3) {
                negate_exponent = true;
                step = 4;
            } else {
                goto finish;
            }
            break;

        case '+':
            if (step == 0 || step == 3) {
                step++;
            } else {
                goto finish;
            }
            break;

        case '.':
            if (step > 1) {
                goto finish;
            }
            step = 2;
            break;

        case 'D':
        case 'E':
        case 'F':
        case 'd':
        case 'e':
        case 'f':
            if (step > 2) {
                goto finish;
            }
            step = 3;
            break;

        default:
            if (c >= '0' && c <= '9') {
                auto digit = c - '0';

                if (step <= 1) { // before decimal point
                    step = 1;
                    if (num_significant_digits || c > '0') {
                        most_significant_digit_position++;
                        qbs_val_significant_digits[num_significant_digits] = c;
                        num_significant_digits++;

                        // Overflow protection for uint64_t
                        if (value.i > (UINT64_MAX / 10) || (value.i == (UINT64_MAX / 10) && unsigned(digit) > (UINT64_MAX % 10))) {
                            value.i = UINT64_MAX;
                        } else {
                            value.i = value.i * 10 + digit;
                        }
                    }
                } else if (step == 2) { // after decimal point
                    if (num_significant_digits == 0 && c == '0') {
                        most_significant_digit_position--;
                    }
                    if (num_significant_digits || c > '0') {
                        qbs_val_significant_digits[num_significant_digits] = c;
                        num_significant_digits++;
                    }
                } else if (step >= 3) { // exponent handling
                    step = 4;
                    if (num_exponent_digits || c > '0') {
                        if (num_exponent_digits >= 18) {
                            goto finish;
                        }
                        exponent_value = exponent_value * 10 + digit; // precalculate
                        num_exponent_digits++;
                    }
                }
            } else {
                goto finish; // invalid character detected
            }
        }
    }

finish:

    // Check for all-zero mantissa
    if (num_significant_digits == 0) {
        return 0;
    }

    // Handle cases where exponent is zero and there is no decimal part
    if (exponent_value == 0 && num_significant_digits == most_significant_digit_position) {
        if constexpr (std::is_same_v<T, uint64_t>) {
            return value.i;
        } else {
            if (!negate && value.i > INT64_MAX) {
                return T(INT64_MAX);
            } else if (negate && value.i > (uint64_t)INT64_MAX + 1) {
                return T(INT64_MIN);
            }

            return negate ? -(int64_t)value.i : (int64_t)value.i;
        }
    }

    // Normalize number (change 123.456E2 to 1.23456E4, or 123.456 to 1.23456E2)
    if (negate_exponent) {
        exponent_value = -exponent_value;
    }

    exponent_value += most_significant_digit_position - 1;

    i = 0;
    // Build a floating-point number in ASCII format
    if (negate) {
        qbs_val_built_number[i] = '-';
        i++;
    }

    if (num_significant_digits) {
        // Build normalized mantissa
        for (auto i2 = 0; i2 < num_significant_digits; i2++) {
            if (i2 == 1) {
                qbs_val_built_number[i] = '.';
                i++;
            }
            qbs_val_built_number[i] = qbs_val_significant_digits[i2];
            i++;
        }
        qbs_val_built_number[i] = 'E';
        i++;
        // Add exponent
        i += sprintf((char *)&qbs_val_built_number[i], "%i", exponent_value);
    } else {
        qbs_val_built_number[i] = '0';
        i++;
    }

    qbs_val_built_number[i] = '\0'; // null-terminate

#ifdef QB64_MINGW
    __mingw_sscanf(qbs_val_built_number, "%Lf", &value.f);
#else
    sscanf(qbs_val_built_number, "%Lf", &value.f);
#endif

    return value.f;

non_decimal: // handle hexadecimal, binary, and octal cases

    if (i >= (s->len - 2)) {
        return 0;
    }

    int64_t hex_digits = 0;

    c = (char)s->chr[i + 1];

    if ((c == 'H') || (c == 'h')) { // hexadecimal
        for (i = i + 2; i < s->len; i++) {
            c = (char)s->chr[i];

            // Check if character is a valid hex digit
            if ((c >= '0' && c <= '9')) {
                c -= '0';
            } else if ((c >= 'A' && c <= 'F')) {
                c -= 'A' - 10;
            } else if ((c >= 'a' && c <= 'f')) {
                c -= 'a' - 10;
            } else {
                break; // exit on invalid character
            }

            value.i = (value.i << 4) | c;

            if (hex_digits || c) {
                hex_digits++;
            }

            if (hex_digits > 16) {
                error(6);
                return 0;
            }
        }

        return value.i;
    } else if ((c == 'B') || (c == 'b')) { // binary
        for (i = i + 2; i < s->len; i++) {
            c = (char)s->chr[i];

            if (c == '0' || c == '1') {
                c -= '0';

                value.i = (value.i << 1) | c;

                if (hex_digits || c) {
                    hex_digits++;
                }

                if (hex_digits > 64) {
                    error(6);
                    return 0;
                }
            } else {
                break;
            }
        }

        return value.i;
    } else if ((c == 'O') || (c == 'o')) { // octal
        for (i = i + 2; i < s->len; i++) {
            c = (char)s->chr[i];

            if ((c >= '0') && (c <= '7')) {
                c -= '0';

                value.i = (value.i << 3) | c;

                if (hex_digits || c) {
                    hex_digits++;
                }

                if (hex_digits >= 22) {
                    if ((hex_digits > 22) || (s->chr[i - 21] > '1')) {
                        error(6);
                        return 0;
                    }
                }
            } else {
                break;
            }
        }

        return value.i;
    }

    return 0; // & followed by unknown
}

// We only need to instantiate the template for the types we need
template int64_t qbs_val<int64_t>(qbs *);
template uint64_t qbs_val<uint64_t>(qbs *);
template long double qbs_val<long double>(qbs *);
