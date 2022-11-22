
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "http.h"

const char *example_result =
"<!doctype html>\n"
"<html>\n"
"<head>\n"
"    <title>Example Domain</title>\n"
"\n"
"    <meta charset=\"utf-8\" />\n"
"    <meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" />\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
"    <style type=\"text/css\">\n"
"    body {\n"
"        background-color: #f0f0f2;\n"
"        margin: 0;\n"
"        padding: 0;\n"
"        font-family: -apple-system, system-ui, BlinkMacSystemFont, \"Segoe UI\", \"Open Sans\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;\n"
"        \n"
"    }\n"
"    div {\n"
"        width: 600px;\n"
"        margin: 5em auto;\n"
"        padding: 2em;\n"
"        background-color: #fdfdff;\n"
"        border-radius: 0.5em;\n"
"        box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);\n"
"    }\n"
"    a:link, a:visited {\n"
"        color: #38488f;\n"
"        text-decoration: none;\n"
"    }\n"
"    @media (max-width: 700px) {\n"
"        div {\n"
"            margin: 0 auto;\n"
"            width: auto;\n"
"        }\n"
"    }\n"
"    </style>    \n"
"</head>\n"
"\n"
"<body>\n"
"<div>\n"
"    <h1>Example Domain</h1>\n"
"    <p>This domain is for use in illustrative examples in documents. You may use this\n"
"    domain in literature without prior coordination or asking for permission.</p>\n"
"    <p><a href=\"https://www.iana.org/domains/example\">More information...</a></p>\n"
"</div>\n"
"</body>\n"
"</html>\n"
;

void test_http() {
    size_t expected_result_len = strlen(example_result);
    const char *urls[] = {
        "http://www.example.com",
        "https://www.example.com",
        "HTTPS://WWW.EXAMPLE.COM",
        "httP://wwW.example.com",
        "www.example.com",
        "http://www.example.com:80",
        "https://www.example.com:443",
        NULL
    };

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
        test_assert_ints_with_name(*url, 0, err);

        test_assert_ints_with_name(*url, expected_result_len, len);

        err = libqb_http_close(1);

        test_assert_ints_with_name(*url, 0, err);
    }
}

int main() {
    libqb_http_init();

    int ret;
    struct unit_test tests[] = {
        { test_http, "http" },
    };

    ret = run_tests("http", tests, sizeof(tests) / sizeof(*tests));

    return ret;
}
