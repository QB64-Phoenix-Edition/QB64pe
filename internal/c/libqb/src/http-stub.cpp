
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "http.h"

void libqb_http_init() {}

void libqb_http_stop() {}

int libqb_http_open(const char *url, int handle) {
    (void)url;
    (void)handle;
    return -1;
}

int libqb_http_close(int handle) {
    (void)handle;
    return -1;
}

int libqb_http_connected(int handle) {
    (void)handle;
    return -1;
}

int libqb_http_get_length(int handle, size_t *length) {
    (void)handle;
    *length = 0;
    return -1;
}

int libqb_http_get(int handle, char *buf, size_t *length) {
    (void)handle;
    (void)buf;
    (void)length;
    return -1;
}

int libqb_http_get_fixed(int id, char *buf, size_t length) {
    (void)id;
    (void)buf;
    (void)length;
    return -1;
}

int libqb_http_get_content_length(int id, uint64_t *ptr) {
    (void)id;
    (void)ptr;
    return -1;
}

int libqb_http_get_status_code(int id) {
    (void)id;
    return -1;
}

const char *libqb_http_get_url(int handle) {
    (void)handle;
    return NULL;
}
