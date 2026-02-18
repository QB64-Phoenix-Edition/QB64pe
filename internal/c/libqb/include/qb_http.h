#ifndef INCLUDE_LIBQB_HTTP_H
#define INCLUDE_LIBQB_HTTP_H

#include <stdint.h>

// Initialize the HTTP system
void libqb_http_init();
void libqb_http_stop();

// All of these functions return 0 on success, and a negative error code on failure.

// Handle is provided and should be unique. Used to identify this connection
int libqb_http_open(const char *url, int handle);
int libqb_http_close(int handle);

int libqb_http_connected(int handle);

// Get length of bytes waiting to be read.
//
// Note that more bytes may come in after calling function, but you're guaranteed to at least have this many bytes
int libqb_http_get_length(int handle, size_t *length);

// Gets the value from the Content-Length HTTP header. If none was provided, returns an error
int libqb_http_get_content_length(int handle, uint64_t *length);

// Returns positive status code. -1 indicates there was none (Ex. Connection was unsuccessful)
int libqb_http_get_status_code(int handle);

// Returns the "effective url" as reported by curl, it indicates the location
// actually connected to after redirects and such.
//
// Returns NULL if it could not be resolved. Returned string is only valid for
// the life of this handle.
const char *libqb_http_get_url(int handle);

// Reads up to length bytes into buf. Length is modified if less bytes than requested are returned
int libqb_http_get(int handle, char *buf, size_t *length);

// Returns an error if less than length bytes are available to read
int libqb_http_get_fixed(int handle, char *buf, size_t length);

#endif
