$Console:Only
ON ERROR GOTO errorhand

h& = _OpenClient("https://www.example.com")
Print h&

System

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
