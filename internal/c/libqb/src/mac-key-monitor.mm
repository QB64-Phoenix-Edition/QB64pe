
#include "libqb-common.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "Cocoa/Cocoa.h"
#include <CoreFoundation/CoreFoundation.h>

#include "keyhandler.h"
#include "mac-key-monitor.h"

void mac_register_key_handler()
{
    [NSEvent addLocalMonitorForEventsMatchingMask:NSFlagsChangedMask
                                          handler:^NSEvent *(NSEvent *event) {
                                            // notes on bitfields:
                                            // if ([event modifierFlags] == 131330) keydown_vk(VK+QBVK_LSHIFT);// 100000000100000010
                                            // if ([event modifierFlags] == 131332) keydown_vk(VK+QBVK_RSHIFT);// 100000000100000100
                                            // if ([event modifierFlags] == 262401) keydown_vk(VK+QBVK_LCTRL); //1000000000100000001
                                            // if ([event modifierFlags] == 270592) keydown_vk(VK+QBVK_RCTRL); //1000010000100000000
                                            // if ([event modifierFlags] == 524576) keydown_vk(VK+QBVK_LALT); //10000000000100100000
                                            // if ([event modifierFlags] == 524608) keydown_vk(VK+QBVK_RALT); //10000000000101000000
                                            // caps lock                                                      //   10000000100000000

                                            int x = [event modifierFlags];

                                            if (x & (1 << 0)) {
                                                if (!keyheld(VK + QBVK_LCTRL))
                                                    keydown_vk(VK + QBVK_LCTRL);
                                            } else {
                                                if (keyheld(VK + QBVK_LCTRL))
                                                    keyup_vk(VK + QBVK_LCTRL);
                                            }
                                            if (x & (1 << 13)) {
                                                if (!keyheld(VK + QBVK_RCTRL))
                                                    keydown_vk(VK + QBVK_RCTRL);
                                            } else {
                                                if (keyheld(VK + QBVK_RCTRL))
                                                    keyup_vk(VK + QBVK_RCTRL);
                                            }

                                            if (x & (1 << 1)) {
                                                if (!keyheld(VK + QBVK_LSHIFT))
                                                    keydown_vk(VK + QBVK_LSHIFT);
                                            } else {
                                                if (keyheld(VK + QBVK_LSHIFT))
                                                    keyup_vk(VK + QBVK_LSHIFT);
                                            }
                                            if (x & (1 << 2)) {
                                                if (!keyheld(VK + QBVK_RSHIFT))
                                                    keydown_vk(VK + QBVK_RSHIFT);
                                            } else {
                                                if (keyheld(VK + QBVK_RSHIFT))
                                                    keyup_vk(VK + QBVK_RSHIFT);
                                            }

                                            if (x & (1 << 5)) {
                                                if (!keyheld(VK + QBVK_LALT))
                                                    keydown_vk(VK + QBVK_LALT);
                                            } else {
                                                if (keyheld(VK + QBVK_LALT))
                                                    keyup_vk(VK + QBVK_LALT);
                                            }
                                            if (x & (1 << 6)) {
                                                if (!keyheld(VK + QBVK_RALT))
                                                    keydown_vk(VK + QBVK_RALT);
                                            } else {
                                                if (keyheld(VK + QBVK_RALT))
                                                    keyup_vk(VK + QBVK_RALT);
                                            }

                                            if (x & (1 << 16)) {
                                                if (!keyheld(VK + QBVK_CAPSLOCK))
                                                    keydown_vk(VK + QBVK_CAPSLOCK);
                                            } else {
                                                if (keyheld(VK + QBVK_CAPSLOCK))
                                                    keyup_vk(VK + QBVK_CAPSLOCK);
                                            }

                                            return event;
                                          }];
}
