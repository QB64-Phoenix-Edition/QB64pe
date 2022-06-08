
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test.h"

static int cur_test_count;
static int total_test_count = 0;
static const char *current_test;
static const char *current_mod;
static int cur_failed_tests;

int run_tests(const char *mod_name, struct unit_test *tests, int test_count)
{
    int i;
    int error_count = 0;
    int errs;
    current_mod = mod_name;

    printf("==== Starting tests for %s ====\n", mod_name);

    for (i = 0; i < test_count; i++) {
        total_test_count++;

        cur_test_count = 0;
        current_test = tests[i].name;

        printf("== #%d: %s ==\n", total_test_count, tests[i].name);

        cur_failed_tests = 0;
        (tests + i)->test ();
        errs = cur_failed_tests;

        printf("== Result: ");
        if (errs != 0)
            printf(COLOR_RED "FAIL -> %d" COLOR_RESET, errs);
        else
            printf(COLOR_GREEN "PASS" COLOR_RESET);

        printf(" ==\n");

        error_count += errs;
    }

    printf("==== Finished tests for %s ====\n", mod_name);
    printf("==== Result: ");
    if (error_count == 0)
        printf(COLOR_GREEN "PASS " COLOR_RESET);
    else
        printf(COLOR_RED "FAIL -> %d " COLOR_RESET, error_count);

    printf("====\n");

    return error_count;
}

void assert_true(const char *arg, const char *func, int cond)
{
    cur_test_count++;
    printf(" [%02d:%03d] %s: %s: ", total_test_count, cur_test_count, func, arg);
    if (cond)
        printf(COLOR_GREEN "PASS" COLOR_RESET);
    else
        printf(COLOR_RED "FAIL" COLOR_RESET);

    printf("\n");

    cur_failed_tests += !cond;
}

void assert_with_name(const char *name, const char *arg, const char *func, int cond)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s: \"%s\"", name, arg);
    assert_true(buf, func, cond);
}

// Displays a buffer of memory in a readable format
static void dump_mem(const char *buf, size_t len, uintptr_t base_addr)
{
    char strbuf[200], strbuf2[200] = { 0 };
    char *cur_b, *start, *bufend, *to_print;
    const unsigned char *b = (const unsigned char *)buf;
    int i = 0, j, skipping = 0;

    cur_b = strbuf;
    start = strbuf;

    for (; i < len; i += 16) {
        bufend = cur_b + sizeof(strbuf);
        cur_b += snprintf(cur_b, bufend - cur_b,  "%08x  ", (i) + base_addr);

        for (j = i; j < i + 16; j++) {
            if (j < len)
                cur_b += snprintf(cur_b, bufend - cur_b, "%02x ", (const unsigned int)b[j]);
            else
                cur_b += snprintf(cur_b, bufend - cur_b, "   ");

            if (j - i == 7)
                *(cur_b++) = ' ';
        }

        cur_b += snprintf(cur_b, bufend - cur_b, " |");
        for (j = i; j < i + 16 && j < len; j++)
            if (b[j] > 31 && b[j] <= 127)
                cur_b += sprintf(cur_b, "%c", b[j]);
            else
                *(cur_b++) = '.';

        cur_b += sprintf(cur_b, "|\n");

        to_print = start;

        if (start == strbuf)
            start = strbuf2;
        else
            start = strbuf;

        cur_b = start;

        /* This logic skips duplicate lines, printing '...' instead
         *
         * The 12 magic number is just so we don't compare the printed address,
         * which is in '0x00000000' format at the beginning of the string */
        if (strcmp(strbuf + 12, strbuf2 + 12) != 0) {
            if (skipping == 1)
                printf("%s", start);
            else if (skipping == 2)
                printf("...\n");
            skipping = 0;
            printf("%s", to_print);
        } else if (skipping >= 1) {
            skipping = 2;
        } else {
            skipping = 1;
        }
    }

    if (skipping) {
        printf("...\n");
        printf("%s", to_print);
    }
}

void assert_buffers_equal_with_name(const char *name, const char *func, const char *arg1, const char *buf1, const char *arg2, const char *buf2, size_t length)
{
    int result = memcmp(buf1, buf2, length);
    char buf[256];
    if (name)
        snprintf(buf, sizeof(buf), "%s: \"memcmp(%s, %s, %zd) == 0\"", name, arg1, arg2, length);
    else
        snprintf(buf, sizeof(buf), "\"memcmp(%s, %s, %zd) == 0\"", arg1, arg2, length);

    assert_true(buf, func, result == 0);

    if (result == 0)
        return;

    // Print differences
    printf("    Expected: %s:\n", arg1);
    dump_mem(buf1, length, 0);

    printf("    Actual: %s:\n", arg2);
    dump_mem(buf2, length, 0);
}

void assert_ints_equal_with_name(const char *name, const char *func, const char *arg1, uint64_t int1, const char *arg2, uint64_t int2)
{
    int result = int1 == int2;
    char buf[256];

    if (name)
        snprintf(buf, sizeof(buf), "%s: \"%s == %s\"", name, arg1, arg2);
    else
        snprintf(buf, sizeof(buf), "\"%s == %s\"", arg1, arg2);

    assert_true(buf, func, result);

    if (result)
        return;

    // Print differences
    printf("    Expected: %s = %lld\n", arg1, int1);
    printf("    Actual:   %s = %lld\n", arg2, int2);
}
