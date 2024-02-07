'this is a test for $INCLUDEONCE behavior

$CONSOLE
_DEST _CONSOLE

PRINT "This prints from the test.bas main program."

'$INCLUDE: 'incl.bm'
'$INCLUDE: 'once.bm'
'$INCLUDE: 'incl.bm'

SYSTEM

