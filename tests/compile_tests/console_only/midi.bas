$CONSOLE:ONLY
ON ERROR GOTO errorhand

CHDIR _STARTDIR$

_MIDISOUNDBANK "./test-soundfont.sf2"

handle = _SNDOPEN("./midi.mid")
PRINT handle;

SYSTEM

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
