$CONSOLE:ONLY

' We set 'Continue' behavior via QB64PE_NOPROMPT=Continue, this should allow
' the program to continue past the ERROR 100
ERROR 100
PRINT "This line should print."

SYSTEM
