$CONSOLE:ONLY

$IF WIN THEN
$ELSE

' We can't do _SCREENIMAGE on Linux or Mac OS build agents, but we can still test
' that it compiles
PRINT -11;
SYSTEM

$END IF

i& = _SCREENIMAGE
PRINT i&;
SYSTEM
