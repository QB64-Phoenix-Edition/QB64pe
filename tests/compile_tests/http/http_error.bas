$Unstable:Http
$Console:Only
ON ERROR GOTO errorhand

' This domain isn't valid at all, an error is triggered
h& = _OpenClient("https://thisisabaddomain")
Print h&

' This gives back a 404, but the connection is still successful in that
' situation.
h& = _OpenClient("https://www.example.com/unknownUrl")
Print h&
Print _StatusCode(h&)

System

errorhand:
PRINT ERR; ERL
RESUME NEXT
