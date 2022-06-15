#ifndef _TEST_H_
#define _TEST_H_

#include <stdint.h>

struct unit_test {
    void (*test)();
    const char *name;
};

#define COLOR_DEFAULT      ""
#define COLOR_RESET        "\033[m"
#define COLOR_BOLD         "\033[1m"
#define COLOR_RED          "\033[31m"
#define COLOR_GREEN        "\033[32m"
#define COLOR_YELLOW       "\033[33m"
#define COLOR_BLUE         "\033[34m"
#define COLOR_MAGENTA      "\033[35m"
#define COLOR_CYAN         "\033[36m"
#define COLOR_BOLD_RED     "\033[1;31m"
#define COLOR_BOLD_GREEN   "\033[1;32m"
#define COLOR_BOLD_YELLOW  "\033[1;33m"
#define COLOR_BOLD_BLUE    "\033[1;34m"
#define COLOR_BOLD_MAGENTA "\033[1;35m"
#define COLOR_BOLD_CYAN    "\033[1;36m"
#define COLOR_BG_RED       "\033[41m"
#define COLOR_BG_GREEN     "\033[42m"
#define COLOR_BG_YELLOW    "\033[43m"
#define COLOR_BG_BLUE      "\033[44m"
#define COLOR_BG_MAGENTA   "\033[45m"
#define COLOR_BG_CYAN      "\033[46m"

#define test_assert(cond) assert_true(#cond, __func__, !!(cond))
#define test_assert_with_name(name, cond) assert_with_name(name, #cond, __func__, !!(cond))

#define test_assert_ints(int1, int2) assert_ints_equal_with_name(NULL, __func__, #int1, (int1), #int2, (int2))
#define test_assert_ints_with_name(name, int1, int2) assert_ints_equal_with_name((name), __func__, #int1, (int1), #int2, (int2))

#define test_assert_buffers(buf1, buf2, len) assert_buffers_equal_with_name(NULL, __func__, #buf1, (buf1), #buf2, (buf2), (len))
#define test_assert_buffers_with_name(name, buf1, buf2, len) assert_buffers_equal_with_name((name), __func__, #buf1, (buf1), #buf2, (buf2), (len))

int run_tests(const char *test_mod_name, struct unit_test *, int test_count);

void assert_true(const char *arg, const char *func, int cond);
void assert_with_name(const char *name, const char *arg, const char *func, int cond);
void assert_buffers_equal_with_name(const char *name, const char *func, const char *arg1, const char *buf1, const char *arg2, const char *buf2, size_t length);
void assert_ints_equal_with_name(const char *name, const char *func, const char *arg1, uint64_t int1, const char *arg2, uint64_t int2);

#endif
