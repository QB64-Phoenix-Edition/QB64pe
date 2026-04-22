Option _Explicit
$Console:Only

Dim I As Long
Dim J As Long
Dim Expected As String
ReDim Arr(0 To 1, 0 To 1) As String

For I = 0 To 1
    For J = 0 To 1
        Arr(I, J) = Chr$(65 + I) + Chr$(65 + J)
    Next J
Next I

ReDim _Preserve Arr(0 To 2, 0 To 3) As String

If LBound(Arr, 1) <> 0 Or UBound(Arr, 1) <> 2 Then
    Print "FAIL 2D string bounds dim1"
    System
End If

If LBound(Arr, 2) <> 0 Or UBound(Arr, 2) <> 3 Then
    Print "FAIL 2D string bounds dim2"
    System
End If

For I = 0 To 1
    For J = 0 To 1
        Expected = Chr$(65 + I) + Chr$(65 + J)
        If Arr(I, J) <> Expected Then
            Print "FAIL 2D string preserve"
            System
        End If
    Next J
Next I

If Arr(2, 3) <> "" Then
    Print "FAIL 2D string new elements"
    System
End If

Print "All pass"
System
