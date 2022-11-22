
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include "http.h"

void libqb_http_init() {
}

void libqb_http_stop() {
}

int libqb_http_open(const char *url, int handle) {
    return -1;
}

int libqb_http_close(int handle) {
    return -1;
}

int libqb_http_connected(int handle) {
    return -1;
}

int libqb_http_get_length(int handle, size_t *length) {
    *length = 0;
    return -1;
}

int libqb_http_get(int handle, char *buf, size_t *length) {
    return -1;
}

int libqb_http_get_fixed(int id, char *buf, size_t length) {
    return -1;
}

int libqb_http_get_content_length(int id, uint64_t *ptr) {
    return -1;
}

int libqb_http_get_status_code(int id) {
    return -1;
}

const char *libqb_http_get_url(int handle) {
    return NULL;
}
