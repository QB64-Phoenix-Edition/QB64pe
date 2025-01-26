#ifndef INCLUDE_LIBQB_EVENT_H
#define INCLUDE_LIBQB_EVENT_H

#include <stddef.h>
#include <stdint.h>

#define QB64_EVENT_CLOSE 1
#define QB64_EVENT_KEY 2
#define QB64_EVENT_RELATIVE_MOUSE_MOVEMENT 3
#define QB64_EVENT_FILE_DROP 4

void error(int32_t error_number);

#ifdef __cplusplus
void evnt(uint32_t linenumber, uint32_t inclinenumber = 0, const char *incfilename = NULL);
extern "C" int qb64_custom_event(int event, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, void *p1, void *p2);
#else
void evnt(uint32_t linenumber, uint32_t inclinenumber, const char *incfilename);
int qb64_custom_event(int event, int v1, int v2, int v3, int v4, int v5, int v6, int v7, int v8, void *p1, void *p2);
#endif

extern uint32_t new_error;
extern uint32_t qbevent;

#endif
