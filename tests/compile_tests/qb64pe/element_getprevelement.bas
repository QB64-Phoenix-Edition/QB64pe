Option _Explicit
DEFLNG A-Z
$Console:Only
Dim Debug As Long
Debug = -1

'$include:'../../../source/global/constants.bas'
'$include:'../../../source/utilities/type.bi'
sp = "@" ' Makes the output readable

DIM test$, strIndex AS LONG, Index AS LONG, ele$, i AS LONG

test$ = ""
strIndex = 0
Index = 0

ele$ = getprevelement$(test$, Index, strIndex)
Print "Empty element: "; ele$
Print "strIndex = 1: "; strIndex
Print "Index = -1: "; Index
Print

test$ = "foo"
strIndex = 0
Index = 0

' Should return one element for 'foo' and then Index = -1
For i = 1 To 2
    ele$ = getprevelement$(test$, Index, strIndex)
    Print "element: "; ele$
    Print "strIndex: "; strIndex
    Print "Index: "; Index
Next
Print

test$ = "foo@bar@baz@20202020@&HADDD"
strIndex = 0
Index = 0

' Should return the 5 individual elements, and then Index = -1
For i = 1 To 6
    ele$ = getprevelement$(test$, Index, strIndex)
    Print "element: "; ele$
    Print "strIndex: "; strIndex
    Print "Index: "; Index
Next
Print

test$ = "@@baz@@@&HADDD@"
strIndex = 0
Index = 0

' Make sure the blank elements are considered
For i = 1 To 8
    ele$ = getprevelement$(test$, Index, strIndex)
    Print "element: "; ele$
    Print "strIndex: "; strIndex
    Print "Index: "; Index
Next

System

'$include:'../../../source/utilities/elements.bas'
