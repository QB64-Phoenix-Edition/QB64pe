Option _Explicit
$Console:Only
Option Base 1

Dim I As Long
Dim J As Long
ReDim Arr(2, 2) As Long

For I = 1 To 2
    For J = 1 To 2
        Arr(I, J) = I * 100 + J
    Next J
Next I

ReDim _Preserve Arr(3, 4) As Long

If LBound(Arr, 1) <> 1 Or UBound(Arr, 1) <> 3 Then
    Print "FAIL option base 1 numeric bounds dim1"
    System
End If

If LBound(Arr, 2) <> 1 Or UBound(Arr, 2) <> 4 Then
    Print "FAIL option base 1 numeric bounds dim2"
    System
End If

For I = 1 To 2
    For J = 1 To 2
        If Arr(I, J) <> I * 100 + J Then
            Print "FAIL option base 1 numeric preserve"
            System
        End If
    Next J
Next I

If Arr(3, 4) <> 0 Then
    Print "FAIL option base 1 numeric new elements"
    System
End If

Print "All pass"
System
