'                                _KEYDOWN Keyboard Values
'
'Esc  F1    F2    F3    F4    F5    F6    F7    F8    F9    F10   F11   F12   Sys  ScL Pause
' 27 15104 15360 15616 15872 16128 16384 16640 16896 17152 17408 34048 34304 +316 +302 +019
'`~  1!  2@  3#  4$  5%  6^  7&  8*  9(  0) -_ =+ BkSp   Ins   Hme   PUp   NumL   /     *    -
'126 33  64  35  36  37  94  38  42  40  41 95 43   8   20992 18176 18688 +300   47    42   45
' 96 49  50  51  52  53  54  55  56  57  48 45 61
'Tab Q   W   E   R   T   Y   U   I   O   P  [{  ]}  \|   Del   End   PDn   7Hme  8/^   9PU   + 
' 9  81  87  69  82  84  89  85  73  79  80 123 125 124 21248 20224 20736 18176 18432 18688 43
'   113 119 101 114 116 121 117 105 111 112  91  93  92                    55    56    57 
'CapL   A   S   D   F   G   H   J   K   L   ;:  '" Enter                   4/<-   5    6/->
'+301  65  83  68  70  71  72  74  75  76  58  34  13                     19200 19456 19712  E
'      97 115 100 102 103 104 106 107 108  59  39                          52    53    54    n
'Shift   Z   X   C   V   B   N   M   ,<  .>  /?    Shift       ^           1End  2/V   3PD   t
'+304   90  88  67  86  66  78  77  60  62  63    +303       18432        20224 20480 20736  e
'      122 120  99 118  98 110 109  44  46  47                             49    50    51    r
'Ctrl   Win  Alt     Spacebar      Alt  Win  Menu  Ctrl   <-   V   ->      0Ins        .Del 
'+306  +311 +308       32         +307 +312 +319  +305 19200 20480 19712  20992       21248 13
'                                                                          48          46
'
'         Lower value = LCase/NumLock On __________________ + = add 100000 

CONST KEY_TAB = 9
CONST KEY_ENTER = 13
CONST KEY_EXCLAIM = 33
CONST KEY_ONE = 49
CONST KEY_F1 = 15104
CONST KEY_F10 = 17408

CONST KEY_SHIFT = 16
CONST KEY_CTRL = 17
CONST KEY_ALT = 18

CONST KEY_PAUSE& = 100019
CONST KEY_NUMLOCK& = 100300
CONST KEY_CAPSLOCK& = 100301
CONST KEY_SCROLLOCK& = 100302
CONST KEY_RSHIFT& = 100303
CONST KEY_LSHIFT& = 100304
CONST KEY_RCTRL& = 100305
CONST KEY_LCTRL& = 100306
CONST KEY_RALT& = 100307
CONST KEY_LALT& = 100308
CONST KEY_RMETA& = 100309 'Left 'Apple' key (macOS)
CONST KEY_LMETA& = 100310 'Right 'Apple' key (macOS)
CONST KEY_LSUPER& = 100311 'Left "Windows" key
CONST KEY_RSUPER& = 100312 'Right "Windows"key
CONST KEY_MODE& = 100313 '"AltGr" key
CONST KEY_COMPOSE& = 100314
CONST KEY_HELP& = 100315
CONST KEY_PRINT& = 100316
CONST KEY_SYSREQ& = 100317
CONST KEY_BREAK& = 100318
CONST KEY_MENU& = 100319
CONST KEY_POWER& = 100320
CONST KEY_EURO& = 100321
CONST KEY_UNDO& = 100322
CONST KEY_KP0& = 100256
CONST KEY_KP1& = 100257
CONST KEY_KP2& = 100258
CONST KEY_KP3& = 100259
CONST KEY_KP4& = 100260
CONST KEY_KP5& = 100261
CONST KEY_KP6& = 100262
CONST KEY_KP7& = 100263
CONST KEY_KP8& = 100264
CONST KEY_KP9& = 100265
CONST KEY_KP_PERIOD& = 100266
CONST KEY_KP_DIVIDE& = 100267
CONST KEY_KP_MULTIPLY& = 100268
CONST KEY_KP_MINUS& = 100269
CONST KEY_KP_PLUS& = 100270
CONST KEY_KP_ENTER& = 100271
CONST KEY_KP_INSERT& = 200000
CONST KEY_KP_END& = 200001
CONST KEY_KP_DOWN& = 200002
CONST KEY_KP_PAGE_DOWN& = 200003
CONST KEY_KP_LEFT& = 200004
CONST KEY_KP_MIDDLE& = 200005
CONST KEY_KP_RIGHT& = 200006
CONST KEY_KP_HOME& = 200007
CONST KEY_KP_UP& = 200008
CONST KEY_KP_PAGE_UP& = 200009
CONST KEY_KP_DELETE& = 200010
CONST KEY_SCROLL_LOCK_MODE& = 200011
CONST KEY_INSERT_MODE& = 200012

Const EscScanCode = &H01
Const EnterScancode = &H1C
Const TabScancode = &H0F
Const OneScancode = &H02
Const AScanCode = &H1E ' A key
Const WScanCode = &H11
Const SpaceScanCode = &H39

Const PgUpScanCode = &H49 Or &H8000
Const PgDownScanCode = &H51 Or &H8000
Const HomeScanCode = &H47 Or &H8000
Const InsertScanCode = &H52 Or &H8000
Const DeleteScanCode = &H53 Or &H8000
Const EndScanCode = &H4F Or &H8000

Const LCtrlScancode = &H1D
Const LShiftScancode = &H2A
Const LAltScancode = &H38
Const RCtrlScancode = &H1D Or &H8000 ' Extended
Const RShiftScancode = &H36
Const RAltScancode = &H38 Or &H8000

Const NumLockScancode = &H45
Const ScrollLockScancode = &H45
Const KeypadUpScancode = &H48

Const F1Scancode = &H3B
Const F2Scancode = &H3C
Const F3Scancode = &H3D
Const F4Scancode = &H3E
Const F5Scancode = &H3F
Const F6Scancode = &H40
Const F7Scancode = &H41
Const F8Scancode = &H42
Const F9Scancode = &H43
Const F10Scancode = &H44
Const F11Scancode = &H57
Const F12Scancode = &H58

