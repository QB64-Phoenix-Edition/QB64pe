
#include "libqb-common.h"

#include "datetime.h"
#include "error_handle.h"
#include "event.h"
#include "qbs.h"
#include "rounding.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <thread>

static std::chrono::steady_clock::time_point g_TimeStart;

void clock_init() {
    g_TimeStart = std::chrono::steady_clock::now();
}

int64_t GetTicks() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - g_TimeStart).count();
}

#ifndef QB64_WINDOWS
void Sleep(uint32_t milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
#endif

static uint64_t millis_since_midnight() {
    const auto now = std::chrono::system_clock::now();
    const auto today = std::chrono::floor<std::chrono::days>(now);
    const auto since_midnight = now - today;

    return std::chrono::duration_cast<std::chrono::milliseconds>(since_midnight).count();
}

double func_timer(double accuracy, int32_t passed) {
    if (new_error)
        return 0;

    double result = millis_since_midnight() / 1000.0;

    // Default accuracy = 18.2 Hz
    if (!passed) {
        accuracy = 18.2;
    } else {
        if (accuracy <= 0.0) {
            error(5);
            return 0;
        }
        accuracy = 1.0 / accuracy;
    }

    result *= accuracy;
    result = qbr(result); // rounding
    result /= accuracy;

    if (!passed) {
        float f = result;
        result = f;
    }

    return result;
}

void sub__delay(double seconds) {
    if (new_error)
        return;

    if (seconds < 0 || seconds > 2147483.647) {
        error(5);
        return;
    }

    const double target_ms = seconds * 1000.0;
    const int64_t start = GetTicks();

    while (true) {
        int64_t now = GetTicks();
        double elapsed = now - start;

        if (elapsed >= target_ms)
            break;

        int64_t remaining = static_cast<int64_t>(target_ms - elapsed);
        if (remaining <= 0)
            remaining = 1;

        if (remaining >= 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(9));
            evnt(0); // event polling
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(remaining));
        }
    }
}

void sub__limit(double fps) {
    if (new_error)
        return;

    static double prev = 0;

    if (fps <= 0.0) {
        error(5);
        return;
    }

    const double frame_ms = 1000.0 / fps;
    if (frame_ms > 60000.0) {
        error(5);
        return;
    }

recalculate:
    double now = GetTicks();

    if (prev == 0.0) {
        prev = now;
        return;
    }

    if (now < prev) { // tick counter wrapped
        prev = now;
        return;
    }

    double elapsed = now - prev;

    if (elapsed == frame_ms) {
        prev += frame_ms;
        return;
    }

    if (elapsed < frame_ms) {
        int64_t wait = static_cast<int64_t>(frame_ms - elapsed);
        if (wait <= 0)
            wait = 1;

        if (wait >= 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(9));
            evnt(0);
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait));
        }

        goto recalculate;
    }

    // Too long since last frame
    if (elapsed <= frame_ms + 32.0)
        prev += frame_ms;
    else
        prev = now;
}

void sub_date(qbs *str) {
    if (is_error_pending())
        return;
    // stub
    (void)str;
}

qbs *func_date() {
    // mm-dd-yyyy
    // 0123456789
    static time_t qb64_tm_val;
    static tm *qb64_tm;
    // struct tm {
    //        int tm_sec;     /* seconds after the minute - [0,59] */
    //        int tm_min;     /* minutes after the hour - [0,59] */
    //        int tm_hour;    /* hours since midnight - [0,23] */
    //        int tm_mday;    /* day of the month - [1,31] */
    //        int tm_mon;     /* months since January - [0,11] */
    //        int tm_year;    /* years since 1900 */
    //        int tm_wday;    /* days since Sunday - [0,6] */
    //        int tm_yday;    /* days since January 1 - [0,365] */
    //        int tm_isdst;   /* daylight savings time flag */
    //        };
    static int32_t x, x2, i;
    static qbs *str;
    str = qbs_new(10, 1);
    str->chr[2] = 45;
    str->chr[5] = 45; //-
    time(&qb64_tm_val);
    if (qb64_tm_val == -1) {
        error(5);
        str->len = 0;
        return str;
    }
    qb64_tm = localtime(&qb64_tm_val);
    if (qb64_tm == NULL) {
        error(5);
        str->len = 0;
        return str;
    }
    x = qb64_tm->tm_mon;
    x++;
    i = 0;
    str->chr[i] = x / 10 + 48;
    str->chr[i + 1] = x % 10 + 48;
    x = qb64_tm->tm_mday;
    i = 3;
    str->chr[i] = x / 10 + 48;
    str->chr[i + 1] = x % 10 + 48;
    x = qb64_tm->tm_year;
    x += 1900;
    i = 6;
    x2 = x / 1000;
    x = x - x2 * 1000;
    str->chr[i] = x2 + 48;
    i++;
    x2 = x / 100;
    x = x - x2 * 100;
    str->chr[i] = x2 + 48;
    i++;
    x2 = x / 10;
    x = x - x2 * 10;
    str->chr[i] = x2 + 48;
    i++;
    str->chr[i] = x + 48;
    return str;
}

void sub_time(qbs *str) {
    if (is_error_pending())
        return;
    // stub
    (void)str;
}

qbs *func_time() {
    // 23:59:59 (hh:mm:ss)
    // 01234567
    static time_t qb64_tm_val;
    static tm *qb64_tm;
    // struct tm {
    //        int tm_sec;     /* seconds after the minute - [0,59] */
    //        int tm_min;     /* minutes after the hour - [0,59] */
    //        int tm_hour;    /* hours since midnight - [0,23] */
    //        int tm_mday;    /* day of the month - [1,31] */
    //        int tm_mon;     /* months since January - [0,11] */
    //        int tm_year;    /* years since 1900 */
    //        int tm_wday;    /* days since Sunday - [0,6] */
    //        int tm_yday;    /* days since January 1 - [0,365] */
    //        int tm_isdst;   /* daylight savings time flag */
    //        };
    static int32_t x, i;
    static qbs *str;
    str = qbs_new(8, 1);
    str->chr[2] = 58;
    str->chr[5] = 58; //:
    time(&qb64_tm_val);
    if (qb64_tm_val == -1) {
        error(5);
        str->len = 0;
        return str;
    }
    qb64_tm = localtime(&qb64_tm_val);
    if (qb64_tm == NULL) {
        error(5);
        str->len = 0;
        return str;
    }
    x = qb64_tm->tm_hour;
    i = 0;
    str->chr[i] = x / 10 + 48;
    str->chr[i + 1] = x % 10 + 48;
    x = qb64_tm->tm_min;
    i = 3;
    str->chr[i] = x / 10 + 48;
    str->chr[i + 1] = x % 10 + 48;
    x = qb64_tm->tm_sec;
    i = 6;
    str->chr[i] = x / 10 + 48;
    str->chr[i + 1] = x % 10 + 48;
    return str;
}
