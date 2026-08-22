$CONSOLE:ONLY
'test for writing and reading records to file using static or dynamic nested array. Condition for this is, that string array must have fixed size.
$UNSTABLE:TYPEFIELDS
FF = FREEFILE

'static nested array
TYPE TestStatic
    _STATIC text(9) AS STRING * 23
    _DYNAMIC nr(9) AS _UNSIGNED LONG
    something AS STRING * 7
END TYPE

REDIM a(1) AS TestStatic
rec$ = "Static Nested Record"

FOR g = 0 TO 1
    FOR f = 0 TO 9
        a(g).text(f) = rec$ + STR$(f + 1)
        a(g).nr(f) = g * 10 + f + 1
    NEXT f
    a(g).something = "padding"
NEXT g
IF _FILEEXISTS("test") THEN KILL "test"
OPEN "test" FOR BINARY AS FF
PUT FF, , a() '            write content
REDIM a(1) AS TestStatic ' erase content from array

'? "empty control:"
'FOR g = 0 TO 1
'FOR f = 0 TO 9
'? a(g).text(f); a(g).nr(f); a(g).something
'NEXT f, g
'SLEEP    '                is empty

GET FF, 1, a() '           read content from file
CLOSE FF

FOR g = 0 TO 1
    FOR f = 0 TO 9
        PRINT a(g).text(f); a(g).nr(f); a(g).something
NEXT f, g

IF _FILEEXISTS("test") THEN KILL "test"
SYSTEM

'is possible using GET / PUT for UDT and normal arrays - but STRING must have fixed size!
