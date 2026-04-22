Option _Explicit
$Console:Only
Option Base 1

Dim I As Long
Dim J As Long
Dim ExpectedVar As String
Dim ExpectedFix As String
ReDim VarArr(2, 2) As String
ReDim FixArr(2, 2) As String * 8

For I = 1 To 2
    For J = 1 To 2
        ExpectedVar = Chr$(64 + I) + Chr$(64 + J)
        ExpectedFix = Left$("F" + Chr$(64 + I) + Chr$(64 + J) + "______", 8)
        VarArr(I, J) = ExpectedVar
        FixArr(I, J) = ExpectedFix
    Next J
Next I

ReDim _Preserve VarArr(3, 4) As String
ReDim _Preserve FixArr(3, 4) As String * 8

If LBound(VarArr, 1) <> 1 Or UBound(VarArr, 1) <> 3 Then
    Print "FAIL option base 1 var string bounds dim1"
    System
End If

If LBound(VarArr, 2) <> 1 Or UBound(VarArr, 2) <> 4 Then
    Print "FAIL option base 1 var string bounds dim2"
    System
End If

If LBound(FixArr, 1) <> 1 Or UBound(FixArr, 1) <> 3 Then
    Print "FAIL option base 1 fixed string bounds dim1"
    System
End If

If LBound(FixArr, 2) <> 1 Or UBound(FixArr, 2) <> 4 Then
    Print "FAIL option base 1 fixed string bounds dim2"
    System
End If

For I = 1 To 2
    For J = 1 To 2
        ExpectedVar = Chr$(64 + I) + Chr$(64 + J)
        ExpectedFix = Left$("F" + Chr$(64 + I) + Chr$(64 + J) + "______", 8)
        If VarArr(I, J) <> ExpectedVar Then
            Print "FAIL option base 1 var string preserve"
            System
        End If
        If FixArr(I, J) <> ExpectedFix Then
            Print "FAIL option base 1 fixed string preserve"
            System
        End If
    Next J
Next I

If VarArr(3, 4) <> "" Then
    Print "FAIL option base 1 var string new elements"
    System
End If

Print "All pass"
System
