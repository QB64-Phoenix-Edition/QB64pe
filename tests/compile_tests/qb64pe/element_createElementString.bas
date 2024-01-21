Option _Explicit
DEFLNG A-Z
$Console:Only
Dim Debug As Long
Debug = -1

'$include:'../../../source/global/constants.bas'
'$include:'../../../source/utilities/type.bi'
sp = "@" ' Makes the output readable

Dim i As Long

PRINT "foobar element: " + createElementString$("foobar")

FOR i = 0 to 30
    PRINT "foobar element"; i; ": " + createElementString$("foobar" + CHR$(i) + "baz")
NEXT

FOR i = 126 to 255
    PRINT "foobar element"; i; ": " + createElementString$("foobar" + CHR$(i) + "baz")
NEXT

DIM s$

FOR i = 0 TO 255
    s$ = s$ + CHR$(i)
NEXT
PRINT "all chars: " + createElementString$(s$)

System

'$include:'../../../source/utilities/elements.bas'
