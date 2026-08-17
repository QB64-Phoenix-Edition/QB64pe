$CONSOLE:ONLY

' With QB64PE_NOPROMPT=y, the ERROR 100 should exit the program without waiting
' for any input
ERROR 100
PRINT "This line should not appear"

SYSTEM
