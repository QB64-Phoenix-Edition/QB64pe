Option _Explicit
$Console:Only

Dim I As Long
Dim J As Long
Dim K As Long
Dim L As Long
Dim P As Long
Dim Expected As String
ReDim Arr(0 To 1, 0 To 1, 0 To 1, 0 To 1, 0 To 1) As String

For I = 0 To 1
    For J = 0 To 1
        For K = 0 To 1
            For L = 0 To 1
                For P = 0 To 1
                    Arr(I, J, K, L, P) = Chr$(65 + I) + Chr$(65 + J) + Chr$(65 + K) + Chr$(65 + L) + Chr$(65 + P)
                Next P
            Next L
        Next K
    Next J
Next I

ReDim _Preserve Arr(0 To 2, 0 To 2, 0 To 2, 0 To 2, 0 To 2) As String

If LBound(Arr, 1) <> 0 Or UBound(Arr, 1) <> 2 Then
    Print "FAIL 5D string bounds dim1"
    System
End If

If LBound(Arr, 2) <> 0 Or UBound(Arr, 2) <> 2 Then
    Print "FAIL 5D string bounds dim2"
    System
End If

If LBound(Arr, 3) <> 0 Or UBound(Arr, 3) <> 2 Then
    Print "FAIL 5D string bounds dim3"
    System
End If

If LBound(Arr, 4) <> 0 Or UBound(Arr, 4) <> 2 Then
    Print "FAIL 5D string bounds dim4"
    System
End If

If LBound(Arr, 5) <> 0 Or UBound(Arr, 5) <> 2 Then
    Print "FAIL 5D string bounds dim5"
    System
End If

For I = 0 To 1
    For J = 0 To 1
        For K = 0 To 1
            For L = 0 To 1
                For P = 0 To 1
                    Expected = Chr$(65 + I) + Chr$(65 + J) + Chr$(65 + K) + Chr$(65 + L) + Chr$(65 + P)
                    If Arr(I, J, K, L, P) <> Expected Then
                        Print "FAIL 5D string preserve"
                        System
                    End If
                Next P
            Next L
        Next K
    Next J
Next I

If Arr(2, 2, 2, 2, 2) <> "" Then
    Print "FAIL 5D string new elements"
    System
End If

Print "All pass"
System
