$CONSOLE:ONLY

REDIM a(10) AS STRING * 20

FOR bb = 0 TO 20
    c$ = c$ + CHR$(100 + bb)
NEXT bb

FOR b = 0 TO 10
    a(b) = c$
NEXT

ff = FREEFILE
OPEN "test" FOR BINARY AS ff
PUT ff, , a()
CLOSE ff
ERASE a
REDIM a(10) AS STRING * 20
CLOSE ff

OPEN "test" FOR BINARY AS ff
GET ff, , a()
CLOSE ff

FOR f = 0 TO 10
    PRINT a(f)
NEXT
SYSTEM