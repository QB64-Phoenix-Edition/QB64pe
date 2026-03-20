$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As Integer
End Type

ReDim reference(0 To 3) As Integer
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = -321
reference(1) = 0
reference(2) = 1234
reference(3) = 32000
item(0).Values(0) = -321
item(0).Values(1) = 0
item(0).Values(2) = 1234
item(0).Values(3) = 32000

ReDim _Preserve reference(0 To 3) As Integer
ReDim _Preserve item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 23_redim_preserve_integer.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 23_redim_preserve_integer.bas"
System
