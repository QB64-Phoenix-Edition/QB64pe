$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As Single
End Type

ReDim reference(0 To 3) As Single
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = -1.25
reference(1) = 0
reference(2) = 3.5
reference(3) = 12345.75
item(0).Values(0) = -1.25
item(0).Values(1) = 0
item(0).Values(2) = 3.5
item(0).Values(3) = 12345.75

ReDim _Preserve reference(0 To 3) As Single
ReDim _Preserve item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 29_redim_preserve_single.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 29_redim_preserve_single.bas"
System
