$CONSOLE:ONLY

t$ = "QB64 Phoenix Edition"
PRINT "   Text: "; t$
PRINT "Txt-Len: "; LTRIM$(STR$(LEN(t$)))
PRINT "-----------------------------------------"
PRINT "Adler32: "; RIGHT$("00000000" + HEX$(_ADLER32(t$)), 8)
PRINT "  Crc32: "; RIGHT$("00000000" + HEX$(_CRC32(t$)), 8)
PRINT "    Md5: "; _MD5$(t$)
PRINT
t$ = ""
PRINT "   Text: "; t$
PRINT "Txt-Len: "; LTRIM$(STR$(LEN(t$)))
PRINT "-----------------------------------------"
PRINT "Adler32: "; RIGHT$("00000000" + HEX$(_ADLER32(t$)), 8)
PRINT "  Crc32: "; RIGHT$("00000000" + HEX$(_CRC32(t$)), 8)
PRINT "    Md5: "; _MD5$(t$)

SYSTEM

