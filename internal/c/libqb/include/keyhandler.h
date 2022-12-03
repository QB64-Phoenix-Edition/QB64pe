#ifndef INCLUDE_LIBQB_KEYHANDLER_h
#define INCLUDE_LIBQB_KEYHANDLER_h

#include <stdint.h>

int32_t keyheld(uint32_t x);

void keydown_vk(uint32_t key);
void keyup_vk(uint32_t key);

#define QBK 200000
#define VK 100000
#define UC 1073741824
/* QBK codes:
    200000-200010: Numpad keys with Num-Lock off
    NO_NUMLOCK_KP0=INSERT
    NO_NUMLOCK_KP1=END
    NO_NUMLOCK_KP2=DOWN
    NO_NUMLOCK_KP3=PGDOWN
    NO_NUMLOCK_KP4...
    NO_NUMLOCK_KP5
    NO_NUMLOCK_KP6
    NO_NUMLOCK_KP7
    NO_NUMLOCK_KP8
    NO_NUMLOCK_KP9
    NO_NUMLOCK_KP_PERIOD=DEL
    200011: SCROLL_LOCK_ON
    200012: INSERT_MODE_ON
*/
#define QBK_SCROLL_LOCK_MODE 11
#define QBK_INSERT_MODE 12
#define QBK_CHR0 13
typedef enum {
    QBVK_UNKNOWN = 0,
    QBVK_FIRST = 0,
    QBVK_BACKSPACE = 8,
    QBVK_TAB = 9,
    QBVK_CLEAR = 12,
    QBVK_RETURN = 13,
    QBVK_PAUSE = 19,
    QBVK_ESCAPE = 27,
    QBVK_SPACE = 32,
    QBVK_EXCLAIM = 33,
    QBVK_QUOTEDBL = 34,
    QBVK_HASH = 35,
    QBVK_DOLLAR = 36,
    QBVK_AMPERSAND = 38,
    QBVK_QUOTE = 39,
    QBVK_LEFTPAREN = 40,
    QBVK_RIGHTPAREN = 41,
    QBVK_ASTERISK = 42,
    QBVK_PLUS = 43,
    QBVK_COMMA = 44,
    QBVK_MINUS = 45,
    QBVK_PERIOD = 46,
    QBVK_SLASH = 47,
    QBVK_0 = 48,
    QBVK_1 = 49,
    QBVK_2 = 50,
    QBVK_3 = 51,
    QBVK_4 = 52,
    QBVK_5 = 53,
    QBVK_6 = 54,
    QBVK_7 = 55,
    QBVK_8 = 56,
    QBVK_9 = 57,
    QBVK_COLON = 58,
    QBVK_SEMICOLON = 59,
    QBVK_LESS = 60,
    QBVK_EQUALS = 61,
    QBVK_GREATER = 62,
    QBVK_QUESTION = 63,
    QBVK_AT = 64,
    // Skip uppercase letters
    QBVK_LEFTBRACKET = 91,
    QBVK_BACKSLASH = 92,
    QBVK_RIGHTBRACKET = 93,
    QBVK_CARET = 94,
    QBVK_UNDERSCORE = 95,
    QBVK_BACKQUOTE = 96,
    QBVK_a = 97,
    QBVK_b = 98,
    QBVK_c = 99,
    QBVK_d = 100,
    QBVK_e = 101,
    QBVK_f = 102,
    QBVK_g = 103,
    QBVK_h = 104,
    QBVK_i = 105,
    QBVK_j = 106,
    QBVK_k = 107,
    QBVK_l = 108,
    QBVK_m = 109,
    QBVK_n = 110,
    QBVK_o = 111,
    QBVK_p = 112,
    QBVK_q = 113,
    QBVK_r = 114,
    QBVK_s = 115,
    QBVK_t = 116,
    QBVK_u = 117,
    QBVK_v = 118,
    QBVK_w = 119,
    QBVK_x = 120,
    QBVK_y = 121,
    QBVK_z = 122,
    QBVK_DELETE = 127,
    // End of ASCII mapped QBVKs
    // International QBVKs
    QBVK_WORLD_0 = 160, /* 0xA0 */
    QBVK_WORLD_1 = 161,
    QBVK_WORLD_2 = 162,
    QBVK_WORLD_3 = 163,
    QBVK_WORLD_4 = 164,
    QBVK_WORLD_5 = 165,
    QBVK_WORLD_6 = 166,
    QBVK_WORLD_7 = 167,
    QBVK_WORLD_8 = 168,
    QBVK_WORLD_9 = 169,
    QBVK_WORLD_10 = 170,
    QBVK_WORLD_11 = 171,
    QBVK_WORLD_12 = 172,
    QBVK_WORLD_13 = 173,
    QBVK_WORLD_14 = 174,
    QBVK_WORLD_15 = 175,
    QBVK_WORLD_16 = 176,
    QBVK_WORLD_17 = 177,
    QBVK_WORLD_18 = 178,
    QBVK_WORLD_19 = 179,
    QBVK_WORLD_20 = 180,
    QBVK_WORLD_21 = 181,
    QBVK_WORLD_22 = 182,
    QBVK_WORLD_23 = 183,
    QBVK_WORLD_24 = 184,
    QBVK_WORLD_25 = 185,
    QBVK_WORLD_26 = 186,
    QBVK_WORLD_27 = 187,
    QBVK_WORLD_28 = 188,
    QBVK_WORLD_29 = 189,
    QBVK_WORLD_30 = 190,
    QBVK_WORLD_31 = 191,
    QBVK_WORLD_32 = 192,
    QBVK_WORLD_33 = 193,
    QBVK_WORLD_34 = 194,
    QBVK_WORLD_35 = 195,
    QBVK_WORLD_36 = 196,
    QBVK_WORLD_37 = 197,
    QBVK_WORLD_38 = 198,
    QBVK_WORLD_39 = 199,
    QBVK_WORLD_40 = 200,
    QBVK_WORLD_41 = 201,
    QBVK_WORLD_42 = 202,
    QBVK_WORLD_43 = 203,
    QBVK_WORLD_44 = 204,
    QBVK_WORLD_45 = 205,
    QBVK_WORLD_46 = 206,
    QBVK_WORLD_47 = 207,
    QBVK_WORLD_48 = 208,
    QBVK_WORLD_49 = 209,
    QBVK_WORLD_50 = 210,
    QBVK_WORLD_51 = 211,
    QBVK_WORLD_52 = 212,
    QBVK_WORLD_53 = 213,
    QBVK_WORLD_54 = 214,
    QBVK_WORLD_55 = 215,
    QBVK_WORLD_56 = 216,
    QBVK_WORLD_57 = 217,
    QBVK_WORLD_58 = 218,
    QBVK_WORLD_59 = 219,
    QBVK_WORLD_60 = 220,
    QBVK_WORLD_61 = 221,
    QBVK_WORLD_62 = 222,
    QBVK_WORLD_63 = 223,
    QBVK_WORLD_64 = 224,
    QBVK_WORLD_65 = 225,
    QBVK_WORLD_66 = 226,
    QBVK_WORLD_67 = 227,
    QBVK_WORLD_68 = 228,
    QBVK_WORLD_69 = 229,
    QBVK_WORLD_70 = 230,
    QBVK_WORLD_71 = 231,
    QBVK_WORLD_72 = 232,
    QBVK_WORLD_73 = 233,
    QBVK_WORLD_74 = 234,
    QBVK_WORLD_75 = 235,
    QBVK_WORLD_76 = 236,
    QBVK_WORLD_77 = 237,
    QBVK_WORLD_78 = 238,
    QBVK_WORLD_79 = 239,
    QBVK_WORLD_80 = 240,
    QBVK_WORLD_81 = 241,
    QBVK_WORLD_82 = 242,
    QBVK_WORLD_83 = 243,
    QBVK_WORLD_84 = 244,
    QBVK_WORLD_85 = 245,
    QBVK_WORLD_86 = 246,
    QBVK_WORLD_87 = 247,
    QBVK_WORLD_88 = 248,
    QBVK_WORLD_89 = 249,
    QBVK_WORLD_90 = 250,
    QBVK_WORLD_91 = 251,
    QBVK_WORLD_92 = 252,
    QBVK_WORLD_93 = 253,
    QBVK_WORLD_94 = 254,
    QBVK_WORLD_95 = 255, /* 0xFF */
    // Numeric keypad
    QBVK_KP0 = 256,
    QBVK_KP1 = 257,
    QBVK_KP2 = 258,
    QBVK_KP3 = 259,
    QBVK_KP4 = 260,
    QBVK_KP5 = 261,
    QBVK_KP6 = 262,
    QBVK_KP7 = 263,
    QBVK_KP8 = 264,
    QBVK_KP9 = 265,
    QBVK_KP_PERIOD = 266,
    QBVK_KP_DIVIDE = 267,
    QBVK_KP_MULTIPLY = 268,
    QBVK_KP_MINUS = 269,
    QBVK_KP_PLUS = 270,
    QBVK_KP_ENTER = 271,
    QBVK_KP_EQUALS = 272,
    // Arrows + Home/End pad
    QBVK_UP = 273,
    QBVK_DOWN = 274,
    QBVK_RIGHT = 275,
    QBVK_LEFT = 276,
    QBVK_INSERT = 277,
    QBVK_HOME = 278,
    QBVK_END = 279,
    QBVK_PAGEUP = 280,
    QBVK_PAGEDOWN = 281,
    // Function keys
    QBVK_F1 = 282,
    QBVK_F2 = 283,
    QBVK_F3 = 284,
    QBVK_F4 = 285,
    QBVK_F5 = 286,
    QBVK_F6 = 287,
    QBVK_F7 = 288,
    QBVK_F8 = 289,
    QBVK_F9 = 290,
    QBVK_F10 = 291,
    QBVK_F11 = 292,
    QBVK_F12 = 293,
    QBVK_F13 = 294,
    QBVK_F14 = 295,
    QBVK_F15 = 296,
    // Key state modifier keys
    QBVK_NUMLOCK = 300,
    QBVK_CAPSLOCK = 301,
    QBVK_SCROLLOCK = 302,
    // If more modifiers are added, the window defocus code in qb64_os_event_linux must be altered
    QBVK_RSHIFT = 303,
    QBVK_LSHIFT = 304,
    QBVK_RCTRL = 305,
    QBVK_LCTRL = 306,
    QBVK_RALT = 307,
    QBVK_LALT = 308,
    QBVK_RMETA = 309,
    QBVK_LMETA = 310,
    QBVK_LSUPER = 311,  /* Left "Windows" key */
    QBVK_RSUPER = 312,  /* Right "Windows" key */
    QBVK_MODE = 313,    /* "Alt Gr" key */
    QBVK_COMPOSE = 314, /* Multi-key compose key */
    // Miscellaneous function keys
    QBVK_HELP = 315,
    QBVK_PRINT = 316,
    QBVK_SYSREQ = 317,
    QBVK_BREAK = 318,
    QBVK_MENU = 319,
    QBVK_POWER = 320, /* Power Macintosh power key */
    QBVK_EURO = 321,  /* Some european keyboards */
    QBVK_UNDO = 322,  /* Atari keyboard has Undo */
    QBVK_LAST
} QBVKs;
// Enumeration of valid key mods (possibly OR'd together)
typedef enum {
    KMOD_NONE = 0x0000,
    KMOD_LSHIFT = 0x0001,
    KMOD_RSHIFT = 0x0002,
    KMOD_LCTRL = 0x0040,
    KMOD_RCTRL = 0x0080,
    KMOD_LALT = 0x0100,
    KMOD_RALT = 0x0200,
    KMOD_LMETA = 0x0400,
    KMOD_RMETA = 0x0800,
    KMOD_NUM = 0x1000,
    KMOD_CAPS = 0x2000,
    KMOD_MODE = 0x4000,
    KMOD_RESERVED = 0x8000
} KMODs;
#define KMOD_CTRL (KMOD_LCTRL | KMOD_RCTRL)
#define KMOD_SHIFT (KMOD_LSHIFT | KMOD_RSHIFT)
#define KMOD_ALT (KMOD_LALT | KMOD_RALT)
#define KMOD_META (KMOD_LMETA | KMOD_RMETA)

#endif
