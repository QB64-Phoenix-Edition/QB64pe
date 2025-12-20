$CONSOLE:ONLY
ON ERROR GOTO errorhand

' This domain isn't valid at all, an error is triggered
h& = _OPENCLIENT("https://thisisabaddomain")
PRINT h&

' This gives back a 404, but the connection is still successful in that
' situation.
h& = _OPENCLIENT("https://qb64phoenix.com/qb64_files/nothere.html")
PRINT h&
PRINT _STATUSCODE(h&)

CLOSE h&
SYSTEM

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
