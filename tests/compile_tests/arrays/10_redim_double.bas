' 10_redim_double.bas
Option _Explicit

Type ItemType
    Values(0 To 3) As Double
End Type

ReDim reference(0 To 3) As Double
ReDim item(0) As ItemType
Dim idx As Long

reference(0) = -1.25
reference(1) = 0
reference(2) = 3.5
reference(3) = 123456789.125
item(0).Values(0) = -1.25
item(0).Values(1) = 0
item(0).Values(2) = 3.5
item(0).Values(3) = 123456789.125

ReDim reference(0 To 3) As Double
ReDim item(0).Values(0 To 3)

For idx = 0 To 3
    If item(0).Values(idx) <> reference(idx) Then
        Print "FAIL 10_redim_double.bas: index"; idx; " expected "; reference(idx); " got "; item(0).Values(idx)
        System
    End If
Next idx

Print "PASS 10_redim_double.bas"
System
