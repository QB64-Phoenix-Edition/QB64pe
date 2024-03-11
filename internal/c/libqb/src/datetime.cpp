
#include "libqb-common.h"

#include <sys/time.h>
#include <chrono>

#ifdef QB64_WINDOWS
# include <synchapi.h>
#endif

#include "rounding.h"
#include "event.h"
#include "error_handle.h"
#include "qbs.h"
#include "datetime.h"

#ifdef QB64_MACOSX
#    include <mach/mach_time.h>
#    define ORWL_NANO (+1.0E-9)
#    define ORWL_GIGA UINT64_C(1000000000)
static double orwl_timebase = 0.0;
static uint64_t orwl_timestart = 0;
static int64_t orwl_gettime(void) {
    if (!orwl_timestart) {
        mach_timebase_info_data_t tb = {0};
        mach_timebase_info(&tb);
        orwl_timebase = tb.numer;
        orwl_timebase /= tb.denom;
        orwl_timestart = mach_absolute_time();
    }
    struct timespec t;
    double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
    t.tv_sec = diff * ORWL_NANO;
    t.tv_nsec = diff - (t.tv_sec * ORWL_GIGA);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
#endif

#ifdef QB64_LINUX
static int64_t initial_tick = 0;

void clock_init() {
    // When GetTicks() is called here initial_tick is zero, so as a result
    // GetTicks() returns the original value of the clock.
    initial_tick = GetTicks();
}

int64_t GetTicks() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (tp.tv_sec * 1000 + tp.tv_nsec / 1000000) - initial_tick;
}
#elif defined QB64_MACOSX
int64_t GetTicks() { return orwl_gettime(); }
#else
int64_t GetTicks() { return ((((int64_t)clock()) * ((int64_t)1000)) / ((int64_t)CLOCKS_PER_SEC)); }
#endif

static uint64_t millis_since_midnight() {
    auto currenttime = std::chrono::system_clock::now();

    // Gives us the number of milliseconds past the current second
    uint64_t millis_only = std::chrono::duration_cast<std::chrono::milliseconds>(currenttime.time_since_epoch()).count() % 1000;

    // Convert to time_t and then hour/min/sec. localtime() takes the current
    // timezone into account for us.
    time_t curttime = std::chrono::system_clock::to_time_t(currenttime);
    struct tm *local = localtime(&curttime);

    // Compute current time as number of seconds past midnight
    uint64_t seconds = local->tm_hour * 3600 + local->tm_min * 60 + local->tm_sec;

    return seconds * 1000 + millis_only;
}

double func_timer(double accuracy, int32_t passed) {
    if (new_error)
        return 0;

    double result = (double)millis_since_midnight() / 1000;

    // Adjust result for requested accuracy, or default accuracy.
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
    result = qbr(result);
    result /= accuracy;
    if (!passed) {
        float f = result;
        result = f;
    }

    return result;
}

#ifndef QB64_WINDOWS
void Sleep(uint32_t milliseconds) {
    static uint64_t sec, nsec;
    sec = milliseconds / 1000;
    nsec = (milliseconds % 1000) * 1000000;
    static timespec ts;
    ts.tv_sec = sec;
    ts.tv_nsec = nsec;
    nanosleep(&ts, NULL);
}
#endif

void sub__delay(double seconds) {
    double ms, base, elapsed, prev_now, now; // cannot be static
    base = GetTicks();
    if (new_error)
        return;
    if (seconds < 0) {
        error(5);
        return;
    }
    if (seconds > 2147483.647) {
        error(5);
        return;
    }
    ms = seconds * 1000.0;
    now = base; // force first prev=... assignment to equal base
recalculate:
    prev_now = now;
    now = GetTicks();
    elapsed = now - base;
    if (elapsed < 0) {                  // GetTicks looped
        base = now - (prev_now - base); // calculate new base
    }
    if (elapsed < ms) {
        int64_t wait; // cannot be static
        wait = ms - elapsed;
        if (!wait)
            wait = 1;
        if (wait >= 10) {
            Sleep(9);
            evnt(0); // check for new events
            // recalculate time
            goto recalculate;
        } else {
            Sleep(wait);
        }
    }
}

void sub__limit(double fps) {
    if (new_error)
        return;
    static double prev = 0;
    double ms, now, elapsed; // cannot be static
    if (fps <= 0.0) {
        error(5);
        return;
    }
    ms = 1000.0 / fps;
    if (ms > 60000.0) {
        error(5);
        return;
    } // max. 1 min delay between frames allowed to avoid accidental lock-up of program
recalculate:
    now = GetTicks();
    if (prev == 0.0) { // first call?
        prev = now;
        return;
    }
    if (now < prev) { // value looped?
        prev = now;
        return;
    }
    elapsed = now - prev; // elapsed time since prev

    if (elapsed == ms) {
        prev = prev + ms;
        return;
    }

    if (elapsed < ms) {
        int64_t wait; // cannot be static
        wait = ms - elapsed;
        if (!wait)
            wait = 1;
        if (wait >= 10) {
            Sleep(9);
            evnt(0); // check for new events
        } else {
            Sleep(wait);
        }
        // recalculate time
        goto recalculate;
    }

    // too long since last call, adjust prev to current time
    // minor overshoot up to 32ms is recovered, otherwise time is re-seeded
    if (elapsed <= (ms + 32.0))
        prev = prev + ms;
    else
        prev = now;
}

void sub_date(qbs *date) {
    if (is_error_pending())
        return;
    return; // stub
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
    return; // stub
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
    static int32_t x, x2, i;
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

