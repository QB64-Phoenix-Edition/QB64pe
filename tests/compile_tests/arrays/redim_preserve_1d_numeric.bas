Option _Explicit
$Console:Only

Dim I As Long
ReDim Arr(0 To 2) As Long

For I = 0 To 2
    Arr(I) = (I + 1) * 10
Next I

ReDim _Preserve Arr(0 To 5) As Long

If LBound(Arr, 1) <> 0 Or UBound(Arr, 1) <> 5 Then
    Print "FAIL 1D numeric bounds"
    System
End If

For I = 0 To 2
    If Arr(I) <> (I + 1) * 10 Then
        Print "FAIL 1D numeric preserve"
        System
    End If
Next I

If Arr(3) <> 0 Or Arr(4) <> 0 Or Arr(5) <> 0 Then
    Print "FAIL 1D numeric new elements"
    System
End If

Print "All pass"
System
