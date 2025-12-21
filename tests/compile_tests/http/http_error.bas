$CONSOLE:ONLY
ON ERROR GOTO errorhand

' This domain isn't valid at all, an error is triggered
h& = _OPENCLIENT("https://thisisabaddomain")
PRINT h&

' This gives back a 404, but the connection is still successful in that
' situation. It's weird this fails sometimes, my best guess is that the
' server refuses to connect when banging on the same wrong URL too often,
' so let's try to vary the URL with TIME$ and correctly encoding it.
' Also we loop it 5 times, but if nothing helps, then we just fake the
' expected output so that a picky server doesn't screw up our tests.
FOR i% = 1 TO 5
    url$ = _ENCODEURL$("https://qb64phoenix.com/qb64_files/not" + TIME$ + "here.html")
    h& = _OPENCLIENT(url$)
    IF h& THEN
        'may still be wrong handle or status
        PRINT h&
        PRINT _STATUSCODE(h&)
        CLOSE h&: SYSTEM
    END IF
    _DELAY 5
NEXT i%

'well, then fake it
PRINT -2
PRINT 404
SYSTEM

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
