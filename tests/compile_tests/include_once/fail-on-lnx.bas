'this is a test for $INCLUDEONCE behavior

$CONSOLE:ONLY

$IF LINUX = -1 AND MACOSX = 0 THEN

    PRINT "This prints from the test.bas main program."

    '$INCLUDE: 'incl.bm'
    '$INCLUDE: 'ONCE.bm'
    '$INCLUDE: 'incl.bm'

$ELSE

    PRINT "This is a Linux-only test."

$END IF

SYSTEM

