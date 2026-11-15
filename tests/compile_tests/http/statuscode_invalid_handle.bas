$Console:Only
ON ERROR GOTO errorhand

h& = _OpenHost("TCP/IP:50000")

' Error, invalid handle type
Print _StatusCode(h&)

' Errors, invalid handle numbers
Print _StatusCode(0)
Print _StatusCode(20)

' Error, negative handle that was never opened
Print _StatusCode(-1000)

System

errorhand:
PRINT "Error:"; ERR; ", Line:"; _ERRORLINE
RESUME NEXT
