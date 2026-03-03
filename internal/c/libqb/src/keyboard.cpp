#include "keyboard.h"

void GLUT_KEYBOARD_BUTTON_FUNC(GLUTEmu_KeyboardKey key, int scancode, GLUTEmu_ButtonAction action, int modifiers) {
    // fprintf(stderr, "key: %d, scancode: %d, action: %d, modifiers: %d\n", key, scancode, action, modifiers);

    int qbKey = -1;

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
        // Handle printable keys
        bool isShift = modifiers & GLUTEmu_KeyboardKeyModifier::Shift;
        // bool isControl = modifiers & GLUTEmu_KeyboardKeyModifier::Control;
        // bool isAlt = modifiers & GLUTEmu_KeyboardKeyModifier::Alt;
        // bool isSuper = modifiers & GLUTEmu_KeyboardKeyModifier::Super;
        bool isCapsLock = modifiers & GLUTEmu_KeyboardKeyModifier::CapsLock;
        // bool isNumLock = modifiers & GLUTEmu_KeyboardKeyModifier::NumLock;
        // bool isScrollLock = modifiers & GLUTEmu_KeyboardKeyModifier::ScrollLock;

        if (key >= GLUTEmu_KeyboardKey::A && key <= GLUTEmu_KeyboardKey::Z) {
            qbKey = 'a' + (int(key) - int(GLUTEmu_KeyboardKey::A));
            if (isShift)
                qbKey -= 32;
        } else if (key >= GLUTEmu_KeyboardKey::Zero && key <= GLUTEmu_KeyboardKey::Nine) {
            qbKey = '0' + (int(key) - int(GLUTEmu_KeyboardKey::Zero));
            if (isShift) {
                const char shifted[] = ")!@#$%^&*(";
                qbKey = shifted[qbKey - '0'];
            }
        } else if (key == GLUTEmu_KeyboardKey::Space) {
            qbKey = ' ';
        } else if (key == GLUTEmu_KeyboardKey::Minus) {
            qbKey = '-';
            if (isShift)
                qbKey = '_';
        } else if (key == GLUTEmu_KeyboardKey::Equal) {
            qbKey = '=';
            if (isShift)
                qbKey = '+';
        } else if (key == GLUTEmu_KeyboardKey::LeftBracket) {
            qbKey = '[';
            if (isShift)
                qbKey = '{';
        } else if (key == GLUTEmu_KeyboardKey::RightBracket) {
            qbKey = ']';
            if (isShift)
                qbKey = '}';
        } else if (key == GLUTEmu_KeyboardKey::Backslash) {
            qbKey = '\\';
            if (isShift)
                qbKey = '|';
        } else if (key == GLUTEmu_KeyboardKey::Semicolon) {
            qbKey = ';';
            if (isShift)
                qbKey = ':';
        } else if (key == GLUTEmu_KeyboardKey::Apostrophe) {
            qbKey = '\'';
            if (isShift)
                qbKey = '"';
        } else if (key == GLUTEmu_KeyboardKey::Comma) {
            qbKey = ',';
            if (isShift)
                qbKey = '<';
        } else if (key == GLUTEmu_KeyboardKey::Period) {
            qbKey = '.';
            if (isShift)
                qbKey = '>';
        } else if (key == GLUTEmu_KeyboardKey::Slash) {
            qbKey = '/';
            if (isShift)
                qbKey = '?';
        } else if (key == GLUTEmu_KeyboardKey::GraveAccent) {
            qbKey = '`';
            if (isShift)
                qbKey = '~';
        } else {
            libqb_log_error("Unhandled key: %d (%d)", int(key), scancode);
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
