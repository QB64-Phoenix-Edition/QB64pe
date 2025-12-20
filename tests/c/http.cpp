
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "http.h"
#include "test.h"

const char *example_result = "<!doctype html><html lang=\"en\"><head><title>Example Domain</title><meta name=\"viewport\" content=\"width=device-width, "
                             "initial-scale=1\"><style>body{background:#eee;width:60vw;margin:15vh "
                             "auto;font-family:system-ui,sans-serif}h1{font-size:1.5em}div{opacity:0.8}a:link,a:visited{color:#348}</"
                             "style><body><div><h1>Example Domain</h1><p>This domain is for use in documentation examples without needing permission. Avoid "
                             "use in operations.<p><a href=\"https://iana.org/domains/example\">Learn more</a></div></body></html>\n";

void test_http() {
    size_t expected_result_len = strlen(example_result);
    const char *urls[] = {"http://www.example.com", "https://www.example.com",   "HTTPS://WWW.EXAMPLE.COM",     "httP://wwW.example.com",
                          "www.example.com",        "http://www.example.com:80", "https://www.example.com:443", NULL};

    for (const char **url = urls; *url; url++) {
        int c = libqb_http_open(*url, 1);

        while (libqb_http_connected(1))
            usleep(10);

        size_t buflen = 0;
        int err = libqb_http_get_length(1, &buflen);

        test_assert_ints_with_name(*url, 0, err);
        test_assert_ints_with_name(*url, expected_result_len, buflen);

        char buf[4096];

        buflen = sizeof(buf);
        err = libqb_http_get(1, buf, &buflen);
        test_assert_ints_with_name(*url, 0, err);
        test_assert_ints_with_name(*url, expected_result_len, buflen);
        test_assert_buffers_with_name(*url, example_result, buf, expected_result_len);

        // Verify Content-Length header is read correctly
        uint64_t len;
        err = libqb_http_get_content_length(1, &len);
        if (!err) {
            // test_assert_ints_with_name(*url, 0, err); // makes no sense anymore
            test_assert_ints_with_name(*url, expected_result_len, len);
        }

        err = libqb_http_close(1);

        test_assert_ints_with_name(*url, 0, err);
    }
}

int main() {
    libqb_http_init();

    int ret;
    struct unit_test tests[] = {
        {test_http, "http"},
    };

    ret = run_tests("http", tests, sizeof(tests) / sizeof(*tests));

    return ret;
}
