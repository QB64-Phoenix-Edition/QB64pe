#ifndef INCLUDE_LIBQB_EVENT_H
#define INCLUDE_LIBQB_EVENT_H

#include <stddef.h>

void error(int32_t error_number);
void evnt(uint32_t linenumber, uint32_t inclinenumber = 0, const char *incfilename = NULL);

extern uint32_t new_error;
extern uint32_t qbevent;

#endif
