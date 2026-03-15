' 25_redim_preserve_long.bas
Option _Explicit

Type ItemType
    Values(0 To 3) As Long
End Type

ReDim reference(0 To 3) As Long
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = -200000
reference(1) = 0
reference(2) = 1234567
reference(3) = 214748
item(0).Values(0) = -200000
item(0).Values(1) = 0
item(0).Values(2) = 1234567
item(0).Values(3) = 214748

ReDim _Preserve reference(0 To 3) As Long
ReDim _Preserve item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 25_redim_preserve_long.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 25_redim_preserve_long.bas"
System
