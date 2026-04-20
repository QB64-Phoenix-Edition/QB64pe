$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As String * 8
End Type

ReDim reference(0 To 3) As String * 8
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = "AB"
reference(1) = "CD34"
reference(2) = "EFGH567"
reference(3) = "ZX"
item(0).Values(0) = "AB"
item(0).Values(1) = "CD34"
item(0).Values(2) = "EFGH567"
item(0).Values(3) = "ZX"

ReDim _Preserve reference(0 To 3) As String * 8
ReDim _Preserve item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 32_redim_preserve_fixed_string.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 32_redim_preserve_fixed_string.bas"
System
