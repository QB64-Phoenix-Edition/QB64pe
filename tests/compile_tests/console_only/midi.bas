$CONSOLE:ONLY

CHDIR _STARTDIR$

_MIDISOUNDBANK "./test-soundfont.sf2"

handle = _SNDOPEN("./midi.mid")
PRINT handle;
_DELAY 1

SYSTEM

