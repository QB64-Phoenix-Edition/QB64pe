$CONSOLE:ONLY
ON ERROR GOTO errorhand

' This domain isn't valid at all, an error is triggered
h& = _OPENCLIENT("https://thisisabaddomain")
PRINT h&

' This gives back a 404, but the connection is still successful in that
' situation. It's weird this fails sometimes, my best guess is that the
' server refuses to connect when banging on the same wrong URL too often,
' so let's try to vary the URL with TIME$ and correctly encoding it.
url$ = _ENCODEURL$("https://qb64phoenix.com/qb64_files/not" + TIME$ + "here.html")
h& = _OPENCLIENT(url$)
PRINT h&
PRINT _STATUSCODE(h&)

CLOSE h&
SYSTEM

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
