$CONSOLE:ONLY
OPTION _EXPLICIT

'$Include:'include/soundfont.bi'

DIM h AS LONG: h = _SNDOPEN("2SKIPIX.mid")

PRINT "play time ="; _SNDLEN(h)

SYSTEM

