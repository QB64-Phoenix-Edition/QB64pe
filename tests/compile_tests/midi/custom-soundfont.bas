' This should compile and load the file successfully without using any soundfont

$CONSOLE
$SCREENHIDE
_DEST _CONSOLE
CHDIR _STARTDIR$

$UNSTABLE:MIDI
$MidiSoundFont: "tests/compile_tests/midi/test-soundfont.sf2"

handle = _SNDOPEN("./midi.mid")

PRINT handle;
SYSTEM
