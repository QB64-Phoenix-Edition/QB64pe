$CONSOLE:ONLY
$EMBED:'./test-soundfont.sf2','sf2'

CHDIR _STARTDIR$

_MIDISOUNDBANK _EMBEDDED$("sf2"), "memory, sf2"

handle = _SNDOPEN("./midi.mid")

PRINT handle;

SYSTEM
