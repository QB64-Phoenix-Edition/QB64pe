' 24_redim_preserve_uinteger.bas
Option _Explicit

Type ItemType
    Values(0 To 3) As _Unsigned Integer
End Type

ReDim reference(0 To 3) As _Unsigned Integer
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = 1
reference(1) = 2
reference(2) = 40000
reference(3) = 65535
item(0).Values(0) = 1
item(0).Values(1) = 2
item(0).Values(2) = 40000
item(0).Values(3) = 65535

ReDim _Preserve reference(0 To 3) As _Unsigned Integer
ReDim _Preserve item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 24_redim_preserve_uinteger.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 24_redim_preserve_uinteger.bas"
System
