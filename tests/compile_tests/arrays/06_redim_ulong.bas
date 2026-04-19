$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Values(0 To 3) As _Unsigned Long
End Type

ReDim reference(0 To 3) As _Unsigned Long
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = 1
reference(1) = 200000
reference(2) = 123456789
reference(3) = 4000000000
item(0).Values(0) = 1
item(0).Values(1) = 200000
item(0).Values(2) = 123456789
item(0).Values(3) = 4000000000

ReDim reference(0 To 3) As _Unsigned Long
ReDim item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 06_redim_ulong.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 06_redim_ulong.bas"
System
