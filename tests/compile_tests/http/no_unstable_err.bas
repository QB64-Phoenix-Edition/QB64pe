$Console:Only
ON ERROR GOTO errorhand

h& = _OpenClient("https://www.example.com")
Print h&

System

errorhand:
PRINT ERR; ERL
RESUME NEXT
