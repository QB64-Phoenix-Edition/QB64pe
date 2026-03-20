#include "keyboard.h"

void GLUT_KEYBOARD_BUTTON_FUNC(GLUTEmu_KeyboardKey key, int scancode, GLUTEmu_ButtonAction action, int modifiers) {
    // fprintf(stderr, "key: %d, scancode: %d, action: %d, modifiers: %d\n", key, scancode, action, modifiers);

    int qbKey = -1;

    const bool isNumLock = modifiers & GLUTEmu_KeyboardKeyModifier::NumLock;
    const bool isShift = modifiers & GLUTEmu_KeyboardKeyModifier::Shift;
    const bool isControl = modifiers & GLUTEmu_KeyboardKeyModifier::Control;
    const bool isAlt = modifiers & GLUTEmu_KeyboardKeyModifier::Alt;
    const bool isCapsLock = modifiers & GLUTEmu_KeyboardKeyModifier::CapsLock;
    const bool useKeypadNumber = isAlt || (isNumLock && !isShift);

    switch (key) {
    case GLUTEmu_KeyboardKey::Escape:
        qbKey = QBVK_ESCAPE;
        break;

    case GLUTEmu_KeyboardKey::Enter:
        qbKey = QBVK_RETURN;
        break;

    case GLUTEmu_KeyboardKey::Tab:
        qbKey = QBVK_TAB;
        break;

    case GLUTEmu_KeyboardKey::Backspace:
        qbKey = QBVK_BACKSPACE;
        break;

    case GLUTEmu_KeyboardKey::KP0:
        qbKey = useKeypadNumber ? (VK + QBVK_KP0) : (QBK + 0);
        break;

    case GLUTEmu_KeyboardKey::KP1:
        qbKey = useKeypadNumber ? (VK + QBVK_KP1) : (QBK + 1);
        break;

    case GLUTEmu_KeyboardKey::KP2:
        qbKey = useKeypadNumber ? (VK + QBVK_KP2) : (QBK + 2);
        break;

    case GLUTEmu_KeyboardKey::KP3:
        qbKey = useKeypadNumber ? (VK + QBVK_KP3) : (QBK + 3);
        break;

    case GLUTEmu_KeyboardKey::KP4:
        qbKey = useKeypadNumber ? (VK + QBVK_KP4) : (QBK + 4);
        break;

    case GLUTEmu_KeyboardKey::KP5:
        qbKey = useKeypadNumber ? (VK + QBVK_KP5) : (QBK + 5);
        break;

    case GLUTEmu_KeyboardKey::KP6:
        qbKey = useKeypadNumber ? (VK + QBVK_KP6) : (QBK + 6);
        break;

    case GLUTEmu_KeyboardKey::KP7:
        qbKey = useKeypadNumber ? (VK + QBVK_KP7) : (QBK + 7);
        break;

    case GLUTEmu_KeyboardKey::KP8:
        qbKey = useKeypadNumber ? (VK + QBVK_KP8) : (QBK + 8);
        break;

    case GLUTEmu_KeyboardKey::KP9:
        qbKey = useKeypadNumber ? (VK + QBVK_KP9) : (QBK + 9);
        break;

    case GLUTEmu_KeyboardKey::KPDecimal:
        qbKey = useKeypadNumber ? (VK + QBVK_KP_PERIOD) : (QBK + 10);
        break;

    case GLUTEmu_KeyboardKey::KPDivide:
        qbKey = VK + QBVK_KP_DIVIDE;
        break;

    case GLUTEmu_KeyboardKey::KPMultiply:
        qbKey = VK + QBVK_KP_MULTIPLY;
        break;

    case GLUTEmu_KeyboardKey::KPSubtract:
        qbKey = VK + QBVK_KP_MINUS;
        break;

    case GLUTEmu_KeyboardKey::KPAdd:
        qbKey = VK + QBVK_KP_PLUS;
        break;

    case GLUTEmu_KeyboardKey::KPEnter:
        qbKey = VK + QBVK_KP_ENTER;
        break;

    case GLUTEmu_KeyboardKey::KPEqual:
        qbKey = VK + QBVK_KP_EQUALS;
        break;

    case GLUTEmu_KeyboardKey::Insert:
        qbKey = 0x5200;
        break;

    case GLUTEmu_KeyboardKey::Delete:
        qbKey = 0x5300;
        break;

    case GLUTEmu_KeyboardKey::Right:
        qbKey = 0x4D00;
        break;

    case GLUTEmu_KeyboardKey::Left:
        qbKey = 0x4B00;
        break;

    case GLUTEmu_KeyboardKey::Down:
        qbKey = 0x5000;
        break;

    case GLUTEmu_KeyboardKey::Up:
        qbKey = 0x4800;
        break;

    case GLUTEmu_KeyboardKey::PageUp:
        qbKey = 0x4900;
        break;

    case GLUTEmu_KeyboardKey::PageDown:
        qbKey = 0x5100;
        break;

    case GLUTEmu_KeyboardKey::Home:
        qbKey = 0x4700;
        break;

    case GLUTEmu_KeyboardKey::End:
        qbKey = 0x4F00;
        break;

    case GLUTEmu_KeyboardKey::CapsLock:
        qbKey = VK + QBVK_CAPSLOCK;
        break;

    case GLUTEmu_KeyboardKey::ScrollLock:
        qbKey = VK + QBVK_SCROLLOCK;
        break;

    case GLUTEmu_KeyboardKey::NumLock:
        qbKey = VK + QBVK_NUMLOCK;
        break;

    case GLUTEmu_KeyboardKey::PrintScreen:
        qbKey = VK + QBVK_PRINT;
        break;

    case GLUTEmu_KeyboardKey::Pause:
        qbKey = VK + QBVK_PAUSE;
        break;

    case GLUTEmu_KeyboardKey::F1:
        qbKey = 0x3B00;
        break;

    case GLUTEmu_KeyboardKey::F2:
        qbKey = 0x3C00;
        break;

    case GLUTEmu_KeyboardKey::F3:
        qbKey = 0x3D00;
        break;

    case GLUTEmu_KeyboardKey::F4:
        qbKey = 0x3E00;
        break;

    case GLUTEmu_KeyboardKey::F5:
        qbKey = 0x3F00;
        break;

    case GLUTEmu_KeyboardKey::F6:
        qbKey = 0x4000;
        break;

    case GLUTEmu_KeyboardKey::F7:
        qbKey = 0x4100;
        break;

    case GLUTEmu_KeyboardKey::F8:
        qbKey = 0x4200;
        break;

    case GLUTEmu_KeyboardKey::F9:
        qbKey = 0x4300;
        break;

    case GLUTEmu_KeyboardKey::F10:
        qbKey = 0x4400;
        break;

    case GLUTEmu_KeyboardKey::F11:
        qbKey = 0x8500;
        break;

    case GLUTEmu_KeyboardKey::F12:
        qbKey = 0x8600;
        break;

    case GLUTEmu_KeyboardKey::LeftShift:
        qbKey = VK + QBVK_LSHIFT;
        break;

    case GLUTEmu_KeyboardKey::LeftControl:
        qbKey = VK + QBVK_LCTRL;
        break;

    case GLUTEmu_KeyboardKey::LeftAlt:
        qbKey = VK + QBVK_LALT;
        break;

    case GLUTEmu_KeyboardKey::LeftSuper:
        qbKey = VK + QBVK_LSUPER;
        break;

    case GLUTEmu_KeyboardKey::RightShift:
        qbKey = VK + QBVK_RSHIFT;
        break;

    case GLUTEmu_KeyboardKey::RightControl:
        qbKey = VK + QBVK_RCTRL;
        break;

    case GLUTEmu_KeyboardKey::RightAlt:
        qbKey = VK + QBVK_RALT;
        break;

    case GLUTEmu_KeyboardKey::RightSuper:
        qbKey = VK + QBVK_RSUPER;
        break;

    case GLUTEmu_KeyboardKey::Menu:
        qbKey = VK + QBVK_MENU;
        break;

    default:
        if (key >= GLUTEmu_KeyboardKey::A && key <= GLUTEmu_KeyboardKey::Z) {
            if (isControl && !isAlt) {
                qbKey = int(key) - int(GLUTEmu_KeyboardKey::A) + 1;
                if (isShift) {
                    qbKey = qbKey - 1 + 'A';
                } else {
                    qbKey = qbKey - 1 + 'a';
                }
            } else {
                qbKey = (isShift ^ isCapsLock ? 'A' : 'a') + (int(key) - int(GLUTEmu_KeyboardKey::A));
            }
        } else if (key >= GLUTEmu_KeyboardKey::Zero && key <= GLUTEmu_KeyboardKey::Nine) {
            qbKey = '0' + (int(key) - int(GLUTEmu_KeyboardKey::Zero));
            if (isShift) {
                const char shifted[] = ")!@#$%^&*(";
                qbKey = shifted[qbKey - '0'];
            }
            if (isControl && !isAlt) {
                if (key == GLUTEmu_KeyboardKey::Six) {
                    qbKey = 30;
                }
            }
        } else if (key == GLUTEmu_KeyboardKey::Space) {
            qbKey = ' ';
        } else if (key == GLUTEmu_KeyboardKey::Minus) {
            qbKey = '-';
            if (isShift) {
                qbKey = '_';
            }
            if (isControl && !isAlt) {
                qbKey = 31;
            }
        } else if (key == GLUTEmu_KeyboardKey::Equal) {
            qbKey = '=';
            if (isShift) {
                qbKey = '+';
            }
        } else if (key == GLUTEmu_KeyboardKey::LeftBracket) {
            qbKey = '[';
            if (isShift) {
                qbKey = '{';
            }
            if (isControl && !isAlt) {
                qbKey = 27;
            }
        } else if (key == GLUTEmu_KeyboardKey::RightBracket) {
            qbKey = ']';
            if (isShift) {
                qbKey = '}';
            }
            if (isControl && !isAlt) {
                qbKey = 29;
            }
        } else if (key == GLUTEmu_KeyboardKey::Backslash) {
            qbKey = '\\';
            if (isShift) {
                qbKey = '|';
            }
            if (isControl && !isAlt) {
                qbKey = 28;
            }
        } else if (key == GLUTEmu_KeyboardKey::Semicolon) {
            qbKey = ';';
            if (isShift) {
                qbKey = ':';
            }
        } else if (key == GLUTEmu_KeyboardKey::Apostrophe) {
            qbKey = '\'';
            if (isShift) {
                qbKey = '"';
            }
        } else if (key == GLUTEmu_KeyboardKey::Comma) {
            qbKey = ',';
            if (isShift) {
                qbKey = '<';
            }
        } else if (key == GLUTEmu_KeyboardKey::Period) {
            qbKey = '.';
            if (isShift) {
                qbKey = '>';
            }
        } else if (key == GLUTEmu_KeyboardKey::Slash) {
            qbKey = '/';
            if (isShift) {
                qbKey = '?';
            }
        } else if (key == GLUTEmu_KeyboardKey::GraveAccent) {
            qbKey = '`';
            if (isShift) {
                qbKey = '~';
            }
        }
        break;
    }

    if (qbKey != -1) {
        switch (action) {
        case GLUTEmu_ButtonAction::Pressed:
        case GLUTEmu_ButtonAction::Repeated:
            keydown(qbKey);
            break;

        case GLUTEmu_ButtonAction::Released:
            keyup(qbKey);
            break;
        }
    }
}
