$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarMultiData
    grid(0 To 1, 0 To 1) _Dynamic As String
End Type

Dim result As DynVarMultiData

result = MakeDynVarMulti

If LBound(result.grid, 1) <> -1 Or UBound(result.grid, 1) <> 0 Then
    Print "FAIL multidim first bounds"
    System 1
End If
If LBound(result.grid, 2) <> 2 Or UBound(result.grid, 2) <> 3 Then
    Print "FAIL multidim second bounds"
    System 1
End If
If result.grid(-1, 2) <> "a" Or result.grid(0, 2) <> "b" Then
    Print "FAIL multidim row 2"
    System 1
End If
If result.grid(-1, 3) <> "c" Or result.grid(0, 3) <> "d" Then
    Print "FAIL multidim row 3"
    System 1
End If

Print "PASS Func_return_UDT_t056"
System 0

Function MakeDynVarMulti As DynVarMultiData
    ReDim MakeDynVarMulti.grid(-1 To 0, 2 To 3)
    MakeDynVarMulti.grid(-1, 2) = "a"
    MakeDynVarMulti.grid(0, 2) = "b"
    MakeDynVarMulti.grid(-1, 3) = "c"
    MakeDynVarMulti.grid(0, 3) = "d"
End Function
