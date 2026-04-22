Option _Explicit
$Console:Only

Dim I As Long
Dim J As Long
ReDim Arr(0 To 1, 0 To 1) As Long

For I = 0 To 1
    For J = 0 To 1
        Arr(I, J) = I * 10 + J
    Next J
Next I

ReDim _Preserve Arr(0 To 2, 0 To 3) As Long

If LBound(Arr, 1) <> 0 Or UBound(Arr, 1) <> 2 Then
    Print "FAIL 2D numeric bounds dim1"
    System
End If

If LBound(Arr, 2) <> 0 Or UBound(Arr, 2) <> 3 Then
    Print "FAIL 2D numeric bounds dim2"
    System
End If

For I = 0 To 1
    For J = 0 To 1
        If Arr(I, J) <> I * 10 + J Then
            Print "FAIL 2D numeric preserve"
            System
        End If
    Next J
Next I

If Arr(2, 3) <> 0 Then
    Print "FAIL 2D numeric new elements"
    System
End If

Print "All pass"
System
