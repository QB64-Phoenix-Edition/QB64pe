$Console:Only
ON ERROR GOTO errorhand

' This domain isn't valid at all, an error is triggered
h& = _OpenClient("HTTP:NotARealURL")
Print h&

' Only HTTP URLs are supported, an error is triggered
h& = _OpenClient("HTTP:ftp://example.com")
Print h&

System

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
