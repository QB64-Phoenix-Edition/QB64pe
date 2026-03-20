#include "keyboard.h"

struct KeyMapEntry {
    GLUTEmu_KeyboardKey key;
    int qbKey;
};

struct PunctuationMapEntry {
    GLUTEmu_KeyboardKey key;
    int normal;
    int shifted;
    int control;
};

static constexpr KeyMapEntry kDirectKeyMap[] = {
    {GLUTEmu_KeyboardKey::Escape, QBVK_ESCAPE},
    {GLUTEmu_KeyboardKey::Enter, QBVK_RETURN},
    {GLUTEmu_KeyboardKey::Tab, QBVK_TAB},
    {GLUTEmu_KeyboardKey::Backspace, QBVK_BACKSPACE},
    {GLUTEmu_KeyboardKey::Insert, 0x5200},
    {GLUTEmu_KeyboardKey::Delete, 0x5300},
    {GLUTEmu_KeyboardKey::Right, 0x4D00},
    {GLUTEmu_KeyboardKey::Left, 0x4B00},
    {GLUTEmu_KeyboardKey::Down, 0x5000},
    {GLUTEmu_KeyboardKey::Up, 0x4800},
    {GLUTEmu_KeyboardKey::PageUp, 0x4900},
    {GLUTEmu_KeyboardKey::PageDown, 0x5100},
    {GLUTEmu_KeyboardKey::Home, 0x4700},
    {GLUTEmu_KeyboardKey::End, 0x4F00},
    {GLUTEmu_KeyboardKey::CapsLock, VK + QBVK_CAPSLOCK},
    {GLUTEmu_KeyboardKey::ScrollLock, VK + QBVK_SCROLLOCK},
    {GLUTEmu_KeyboardKey::NumLock, VK + QBVK_NUMLOCK},
    {GLUTEmu_KeyboardKey::PrintScreen, VK + QBVK_PRINT},
    {GLUTEmu_KeyboardKey::Pause, VK + QBVK_PAUSE},
    {GLUTEmu_KeyboardKey::F1, 0x3B00},
    {GLUTEmu_KeyboardKey::F2, 0x3C00},
    {GLUTEmu_KeyboardKey::F3, 0x3D00},
    {GLUTEmu_KeyboardKey::F4, 0x3E00},
    {GLUTEmu_KeyboardKey::F5, 0x3F00},
    {GLUTEmu_KeyboardKey::F6, 0x4000},
    {GLUTEmu_KeyboardKey::F7, 0x4100},
    {GLUTEmu_KeyboardKey::F8, 0x4200},
    {GLUTEmu_KeyboardKey::F9, 0x4300},
    {GLUTEmu_KeyboardKey::F10, 0x4400},
    {GLUTEmu_KeyboardKey::F11, 0x8500},
    {GLUTEmu_KeyboardKey::F12, 0x8600},
    {GLUTEmu_KeyboardKey::KPDivide, VK + QBVK_KP_DIVIDE},
    {GLUTEmu_KeyboardKey::KPMultiply, VK + QBVK_KP_MULTIPLY},
    {GLUTEmu_KeyboardKey::KPSubtract, VK + QBVK_KP_MINUS},
    {GLUTEmu_KeyboardKey::KPAdd, VK + QBVK_KP_PLUS},
    {GLUTEmu_KeyboardKey::KPEnter, VK + QBVK_KP_ENTER},
    {GLUTEmu_KeyboardKey::KPEqual, VK + QBVK_KP_EQUALS},
    {GLUTEmu_KeyboardKey::LeftShift, VK + QBVK_LSHIFT},
    {GLUTEmu_KeyboardKey::LeftControl, VK + QBVK_LCTRL},
    {GLUTEmu_KeyboardKey::LeftAlt, VK + QBVK_LALT},
    {GLUTEmu_KeyboardKey::LeftSuper, VK + QBVK_LSUPER},
    {GLUTEmu_KeyboardKey::RightShift, VK + QBVK_RSHIFT},
    {GLUTEmu_KeyboardKey::RightControl, VK + QBVK_RCTRL},
    {GLUTEmu_KeyboardKey::RightAlt, VK + QBVK_RALT},
    {GLUTEmu_KeyboardKey::RightSuper, VK + QBVK_RSUPER},
    {GLUTEmu_KeyboardKey::Menu, VK + QBVK_MENU},
};

static constexpr PunctuationMapEntry kPunctuationMap[] = {
    {GLUTEmu_KeyboardKey::Minus, '-', '_', 31},        {GLUTEmu_KeyboardKey::Equal, '=', '+', -1},       {GLUTEmu_KeyboardKey::LeftBracket, '[', '{', 27},
    {GLUTEmu_KeyboardKey::RightBracket, ']', '}', 29}, {GLUTEmu_KeyboardKey::Backslash, '\\', '|', 28},  {GLUTEmu_KeyboardKey::Semicolon, ';', ':', -1},
    {GLUTEmu_KeyboardKey::Apostrophe, '\'', '"', -1},  {GLUTEmu_KeyboardKey::Comma, ',', '<', -1},       {GLUTEmu_KeyboardKey::Period, '.', '>', -1},
    {GLUTEmu_KeyboardKey::Slash, '/', '?', -1},        {GLUTEmu_KeyboardKey::GraveAccent, '`', '~', -1},
};

static constexpr char kShiftedDigits[] = ")!@#$%^&*(";

static int constexpr TranslateDirectKey(GLUTEmu_KeyboardKey key) {
    for (const auto &entry : kDirectKeyMap) {
        if (entry.key == key) {
            return entry.qbKey;
        }
    }

    return -1;
}

static int constexpr TranslateKeypadDigitOrDecimal(GLUTEmu_KeyboardKey key, bool useKeypadNumber) {
    if (key >= GLUTEmu_KeyboardKey::KP0 && key <= GLUTEmu_KeyboardKey::KP9) {
        const int offset = int(key) - int(GLUTEmu_KeyboardKey::KP0);
        return useKeypadNumber ? (VK + QBVK_KP0 + offset) : (QBK + offset);
    }

    if (key == GLUTEmu_KeyboardKey::KPDecimal) {
        return useKeypadNumber ? (VK + QBVK_KP_PERIOD) : (QBK + 10);
    }

    return -1;
}

static int constexpr TranslateLetter(GLUTEmu_KeyboardKey key, bool isShift, bool isControl, bool isAlt, bool isCapsLock) {
    if (key < GLUTEmu_KeyboardKey::A || key > GLUTEmu_KeyboardKey::Z) {
        return -1;
    }

    const int offset = int(key) - int(GLUTEmu_KeyboardKey::A);

    if (isControl && !isAlt) {
        int ctrlValue = offset + 1;
        return isShift ? (ctrlValue - 1 + 'A') : (ctrlValue - 1 + 'a');
    }

    return (isShift ^ isCapsLock ? 'A' : 'a') + offset;
}

static int constexpr TranslateDigit(GLUTEmu_KeyboardKey key, bool isShift, bool isControl, bool isAlt) {
    if (key < GLUTEmu_KeyboardKey::Zero || key > GLUTEmu_KeyboardKey::Nine) {
        return -1;
    }

    const int offset = int(key) - int(GLUTEmu_KeyboardKey::Zero);
    int translated = '0' + offset;

    if (isShift) {
        translated = kShiftedDigits[offset];
    }

    if (isControl && !isAlt && key == GLUTEmu_KeyboardKey::Six) {
        translated = 30;
    }

    return translated;
}

static int constexpr TranslatePunctuation(GLUTEmu_KeyboardKey key, bool isShift, bool isControl, bool isAlt) {
    for (const auto &entry : kPunctuationMap) {
        if (entry.key == key) {
            if (isControl && !isAlt && entry.control != -1) {
                return entry.control;
            }

            return isShift ? entry.shifted : entry.normal;
        }
    }

    return -1;
}

static int constexpr TranslatePrintableKey(GLUTEmu_KeyboardKey key, bool isShift, bool isControl, bool isAlt, bool isCapsLock) {
    int translated = TranslateLetter(key, isShift, isControl, isAlt, isCapsLock);
    if (translated != -1) {
        return translated;
    }

    translated = TranslateDigit(key, isShift, isControl, isAlt);
    if (translated != -1) {
        return translated;
    }

    if (key == GLUTEmu_KeyboardKey::Space) {
        return ' ';
    }

    return TranslatePunctuation(key, isShift, isControl, isAlt);
}

static int constexpr TranslateKey(GLUTEmu_KeyboardKey key, bool isShift, bool isControl, bool isAlt, bool isCapsLock, bool useKeypadNumber) {
    int translated = TranslateDirectKey(key);
    if (translated != -1) {
        return translated;
    }

    translated = TranslateKeypadDigitOrDecimal(key, useKeypadNumber);
    if (translated != -1) {
        return translated;
    }

    return TranslatePrintableKey(key, isShift, isControl, isAlt, isCapsLock);
}

void GLUT_KEYBOARD_BUTTON_FUNC(GLUTEmu_KeyboardKey key, int scancode, GLUTEmu_ButtonAction action, int modifiers) {
    (void)scancode;

    const bool isNumLock = modifiers & GLUTEmu_KeyboardKeyModifier::NumLock;
    const bool isShift = modifiers & GLUTEmu_KeyboardKeyModifier::Shift;
    const bool isControl = modifiers & GLUTEmu_KeyboardKeyModifier::Control;
    const bool isAlt = modifiers & GLUTEmu_KeyboardKeyModifier::Alt;
    const bool isCapsLock = modifiers & GLUTEmu_KeyboardKeyModifier::CapsLock;
    const bool useKeypadNumber = isAlt || (isNumLock && !isShift);

    int qbKey = TranslateKey(key, isShift, isControl, isAlt, isCapsLock, useKeypadNumber);

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
