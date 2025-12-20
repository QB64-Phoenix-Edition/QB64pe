$CONSOLE
CHDIR _STARTDIR$

_MIDISOUNDBANK "./test-soundfont.sf2"

handle = _SNDOPEN("./midi.mid")

_DEST _CONSOLE
PRINT handle;
SYSTEM
