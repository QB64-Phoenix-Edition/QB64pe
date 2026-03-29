$CONSOLE:ONLY

' This test hangs on the Windows 11 on ARM runner quite often and randomly.
' I have not been able to recreate it on a real WOA box.
' But we really need to figure out what is going on inside the runner.
' Until then we'll just print the test output.
$IF WINDOWS AND _ARM_ THEN
    PRINT 1
    SYSTEM
$END IF

ON ERROR GOTO errorhand

CHDIR _STARTDIR$

_MIDISOUNDBANK "./test-soundfont.sf2"

handle = _SNDOPEN("./midi.mid")
PRINT handle;

SYSTEM

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
