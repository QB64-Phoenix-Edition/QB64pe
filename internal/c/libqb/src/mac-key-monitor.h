#ifndef INCLUDE_INTERNAL_MAC_KEY_MONITOR_H
#define INCLUDE_INTERNAL_MAC_KEY_MONITOR_H

#ifdef QB64_MACOSX
void mac_register_key_handler();
#else
static inline void mac_register_key_handler() { };
#endif

#endif
