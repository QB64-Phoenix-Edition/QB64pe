Option _Explicit
$Console:Only

Dim I As Long
ReDim Arr(1 To 3) As String

Arr(1) = "alpha"
Arr(2) = "beta"
Arr(3) = "gamma"

ReDim _Preserve Arr(1 To 5) As String

If LBound(Arr, 1) <> 1 Or UBound(Arr, 1) <> 5 Then
    Print "FAIL 1D string bounds"
    System
End If

If Arr(1) <> "alpha" Or Arr(2) <> "beta" Or Arr(3) <> "gamma" Then
    Print "FAIL 1D string preserve"
    System
End If

If Arr(4) <> "" Or Arr(5) <> "" Then
    Print "FAIL 1D string new elements"
    System
End If

Print "All pass"
System
