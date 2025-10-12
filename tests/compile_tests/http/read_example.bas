$CONSOLE:ONLY

h& = _OPENCLIENT("https://www.example.com")
PRINT h&

length~& = LOF(h&)

result$ = ""
WHILE NOT EOF(h&)
    _LIMIT 100
    GET #h&, , s$
    result$ = result$ + s$
WEND


' Strip off the trailing slash if it's there to make the result consistent
url$ = _CONNECTIONADDRESS$(h&)
IF MID$(url$, LEN(url$), 1) = "/" THEN
    url$ = LEFT$(url$, LEN(url$) - 1)
END IF

PRINT "Content-Length: "; length~&
PRINT "Url: "; url$
PRINT "Status Code: "; _STATUSCODE(h&)
PRINT
PRINT result$

CLOSE h&

SYSTEM
