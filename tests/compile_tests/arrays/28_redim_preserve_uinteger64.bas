$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As _Unsigned _Integer64
End Type

ReDim reference(0 To 3) As _Unsigned _Integer64
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = 1
reference(1) = 9000000000
reference(2) = 123456789012
reference(3) = 1844674407370
item(0).Values(0) = 1
item(0).Values(1) = 9000000000
item(0).Values(2) = 123456789012
item(0).Values(3) = 1844674407370

ReDim _Preserve reference(0 To 3) As _Unsigned _Integer64
ReDim _Preserve item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 28_redim_preserve_uinteger64.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 28_redim_preserve_uinteger64.bas"
System
