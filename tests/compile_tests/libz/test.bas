$CONSOLE:ONLY

OPTION _EXPLICIT

CONST LOREM_IPSUM_B64 = "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxpdCwgc2V" + _
    "kIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9yZSBtYWduYSBhbGlxdWEuIFV0IGVuaW0gYW" + _
    "QgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQgZXhlcmNpdGF0aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZ" + _
    "XggZWEgY29tbW9kbyBjb25zZXF1YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhlbmRlcml0IGluIHZvbHVwdGF0" + _
    "ZSB2ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxhIHBhcmlhdHVyLiBFeGNlcHRldXIgc2ludCBvY2NhZWN" + _
    "hdCBjdXBpZGF0YXQgbm9uIHByb2lkZW50LCBzdW50IGluIGN1bHBhIHF1aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIG" + _
    "lkIGVzdCBsYWJvcnVtLg=="

CONST LOREM_IPSUM = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut " + _
    "labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip " + _
    "ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat " + _
    "nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

CONST SIZE_TEST_DATA~& = 1882~&
CONST COMP_TEST_DATA = "eNpz8o1iZwCDKiDOAWIxIBYBYkYGBQZmBlzgPwTBOCQBmKb//0H45tVrFOP9Fy+CzUIzE8SmCB+5fBls" + _
    "DqaZlOOjILMRZlITg8KDFuaCzBw1d9TcUXNHzcVV7tCqnKRZuU6reggAlatSMg=="

IF _STRCMP(_BASE64DECODE$(LOREM_IPSUM_B64), LOREM_IPSUM) = _EQUAL THEN
    PRINT "Pre-encoded base64 decode test passed."
ELSE
    PRINT "Pre-encoded base64 decode test failed!"
END IF

IF _STRCMP(_BASE64DECODE$(_BASE64ENCODE$(LOREM_IPSUM)), LOREM_IPSUM) = _EQUAL THEN
    PRINT "Base64 round trip test passed."
ELSE
    PRINT "Base64 round trip test failed!"
END IF

IF LEN(_INFLATE$(_BASE64DECODE$(COMP_TEST_DATA))) = SIZE_TEST_DATA THEN
    PRINT "Pre-compressed deflate test passed."
ELSE
    PRINT "Pre-compressed deflate test failed!"
END IF

DIM i AS LONG

FOR i = 0 TO 10
    IF _STRCMP(_INFLATE$(_DEFLATE$(LOREM_IPSUM, i)), LOREM_IPSUM) = _EQUAL THEN
        PRINT USING "Deflate round trip test ## passed."; i
    ELSE
        PRINT USING "Deflate round trip test ## failed!"; i
    END IF
NEXT i

SYSTEM
