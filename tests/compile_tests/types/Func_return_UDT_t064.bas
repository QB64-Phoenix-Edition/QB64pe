$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerMultiElemData
    textValue As String
    numberValue As Long
End Type

Type DynOwnerMultiResultData
    grid(0 To 0, 0 To 0) _Dynamic As DynOwnerMultiElemData
End Type

Dim result As DynOwnerMultiResultData

result = MakeDynOwnerMulti

If LBound(result.grid, 1) <> -1 Or UBound(result.grid, 1) <> 0 Then
    Print "FAIL owner multidim first bounds"
    System 1
End If
If LBound(result.grid, 2) <> 2 Or UBound(result.grid, 2) <> 3 Then
    Print "FAIL owner multidim second bounds"
    System 1
End If
If result.grid(-1, 2).textValue <> "a" Or result.grid(-1, 2).numberValue <> 12 Then
    Print "FAIL owner multidim a"
    System 1
End If
If result.grid(0, 2).textValue <> "b" Or result.grid(0, 2).numberValue <> 22 Then
    Print "FAIL owner multidim b"
    System 1
End If
If result.grid(-1, 3).textValue <> "c" Or result.grid(-1, 3).numberValue <> 13 Then
    Print "FAIL owner multidim c"
    System 1
End If
If result.grid(0, 3).textValue <> "d" Or result.grid(0, 3).numberValue <> 23 Then
    Print "FAIL owner multidim d"
    System 1
End If

Print "PASS Func_return_UDT_t064"
System 0

Function MakeDynOwnerMulti As DynOwnerMultiResultData
    ReDim MakeDynOwnerMulti.grid(-1 To 0, 2 To 3)
    MakeDynOwnerMulti.grid(-1, 2).textValue = "a"
    MakeDynOwnerMulti.grid(-1, 2).numberValue = 12
    MakeDynOwnerMulti.grid(0, 2).textValue = "b"
    MakeDynOwnerMulti.grid(0, 2).numberValue = 22
    MakeDynOwnerMulti.grid(-1, 3).textValue = "c"
    MakeDynOwnerMulti.grid(-1, 3).numberValue = 13
    MakeDynOwnerMulti.grid(0, 3).textValue = "d"
    MakeDynOwnerMulti.grid(0, 3).numberValue = 23
End Function
