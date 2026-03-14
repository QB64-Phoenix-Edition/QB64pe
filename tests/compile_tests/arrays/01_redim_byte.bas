$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As _Byte
End Type

ReDim reference(0 To 3) As _Byte
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = -5
reference(1) = 7
reference(2) = 12
reference(3) = 100
item(0).Values(0) = -5
item(0).Values(1) = 7
item(0).Values(2) = 12
item(0).Values(3) = 100

ReDim reference(0 To 3) As _Byte
ReDim item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 01_redim_byte.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 01_redim_byte.bas"
System
