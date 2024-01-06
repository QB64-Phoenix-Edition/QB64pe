Option _Explicit
DEFLNG A-Z
$Console:Only
Dim Debug As Long
Debug = -1

'$include:'../../../source/global/constants.bas'
'$include:'../../../source/utilities/type.bi'
sp = "@" ' Makes the output readable

Dim i As Long, value As String, typ As Long

typ = elementGetStringValue&(createElementString$("foobar"), value)
PRINT "foobar element: " + value

FOR i = 1 to 30
    typ = elementGetStringValue&(createElementString$("foobar" + CHR$(i) + "baz"), value)
    PRINT "foobar element"; i
    PRINT value
NEXT

FOR i = 126 to 255
    typ = elementGetStringValue&(createElementString$("foobar" + CHR$(i) + "baz"), value)
    PRINT "foobar element"; i
    PRINT value
NEXT

DIM s$

FOR i = 0 TO 255
    s$ = s$ + CHR$(i)
NEXT

typ = elementGetStringValue&(createElementString$(s$), value)


PRINT "all chars: ";
FOR i = 1 TO LEN(value)
    PRINT HEX$(ASC(value, i)); " ";
NEXT

System

'$include:'../../../source/utilities/elements.bas'
