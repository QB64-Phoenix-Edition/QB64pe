$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As _Unsigned _Byte
End Type

ReDim reference(0 To 3) As _Unsigned _Byte
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = 5
reference(1) = 7
reference(2) = 12
reference(3) = 200
item(0).Values(0) = 5
item(0).Values(1) = 7
item(0).Values(2) = 12
item(0).Values(3) = 200

ReDim _Preserve reference(0 To 3) As _Unsigned _Byte
ReDim _Preserve item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 22_redim_preserve_ubyte.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 22_redim_preserve_ubyte.bas"
System
