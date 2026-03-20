$CONSOLE:ONLY
Option _Explicit

Type ItemType
    Matrix(0 To 1, 0 To 2) As Long
End Type

ReDim reference(0 To 1, 0 To 2) As Long
ReDim item(0) As ItemType
Dim r As Long
Dim c As Long

For r = 0 To 1
    For c = 0 To 2
        reference(r, c) = r * 100 + c * 7 + 11
        item(0).Matrix(r, c) = r * 100 + c * 7 + 11
    Next c
Next r

ReDim reference(0 To 1, 0 To 2) As Long
ReDim item(0).Matrix(0 To 1, 0 To 2)

For r = 0 To 1
    For c = 0 To 2
        If item(0).Matrix(r, c) <> reference(r, c) Then
            Print "FAIL 13_redim_2d_long.bas: r="; r; " c="; c; " expected "; reference(r, c); " got "; item(0).Matrix(r, c)
            System
        End If
    Next c
Next r

Print "PASS 13_redim_2d_long.bas"
System
