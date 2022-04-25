
'$INCLUDE:'.\global_settings.bas'


$If CONSTANTS_BAS = UNDEFINED Then
    'String SPacer/delimiter constants
    'sp is used as the primary string spacer
    'sp2 & sp3 are used when further delimiation is required
    'for instance, sp2 is used for embedding spacing info for auto-layout by an IDE
    Dim Shared sp As String * 1, sp2 As String * 1, sp3 As String * 1
    sp = Chr$(13): sp2 = Chr$(10): sp3 = Chr$(26)
    Dim Shared sp_asc As Long, sp2_asc As Long, sp3_asc As Long
    sp_asc = Asc(sp): sp2_asc = Asc(sp2): sp3_asc = Asc(sp3)
    If Debug Then sp = Chr$(250): sp2 = Chr$(249): sp3 = Chr$(179) 'makes debug output more readable

    'ASCII codes
    Const ASC_BACKSLASH = 92
    Const ASC_FORWARDSLASH = 47
    Const ASC_LEFTBRACKET = 40
    Const ASC_RIGHTBRACKET = 41
    Const ASC_FULLSTOP = 46
    Const ASC_COLON = 58
    Const ASC_SEMICOLON = 59
    Const ASC_UNDERSCORE = 95
    Const ASC_QUOTE = 34
    Const ASC_LEFTSQUAREBRACKET = 91
    Const ASC_RIGHTSQUAREBRACKET = 93
    Const ASC_QUESTIONMARK = 63

    '_KEYDOWN/_KEYHIT codes
    Const KEY_LSHIFT = 100304
    Const KEY_RSHIFT = 100303
    Const KEY_LCTRL = 100306
    Const KEY_RCTRL = 100305
    Const KEY_LALT = 100308
    Const KEY_RALT = 100307
    Const KEY_LAPPLE = 100310
    Const KEY_RAPPLE = 100309
    Const KEY_F1 = 15104
    Const KEY_F2 = 15360
    Const KEY_F3 = 15616
    Const KEY_F4 = 15872
    Const KEY_F5 = 16128
    Const KEY_F6 = 16384
    Const KEY_F7 = 16640
    Const KEY_F8 = 16896
    Const KEY_F9 = 17152
    Const KEY_F10 = 17408
    Const KEY_F11 = 34048
    Const KEY_F12 = 34304
    Const KEY_INSERT = 20992
    Const KEY_DELETE = 21248
    Const KEY_HOME = 18176
    Const KEY_END = 20224
    Const KEY_PAGEUP = 18688
    Const KEY_PAGEDOWN = 20736
    Const KEY_LEFT = 19200
    Const KEY_RIGHT = 19712
    Const KEY_UP = 18432
    Const KEY_DOWN = 20480
    Const KEY_ESC = 27
    Const KEY_ENTER = 13

    Dim Shared CHR_QUOTE As String: CHR_QUOTE = Chr$(34)
    Dim Shared CHR_TAB As String: CHR_TAB = Chr$(9)
    Dim Shared CRLF As String: CRLF = Chr$(13) + Chr$(10) 'carriage return+line feed
$End If
$Let CONSTANTS_BAS = TRUE
