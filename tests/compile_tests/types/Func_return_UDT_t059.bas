$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerScalarElemData
    textValue As String
    numberValue As Long
End Type

Type DynOwnerScalarResultData
    items(0 To 0) _Dynamic As DynOwnerScalarElemData
End Type

Dim result As DynOwnerScalarResultData

result = MakeDynOwnerScalar

If LBound(result.items) <> -2 Or UBound(result.items) <> 0 Then
    Print "FAIL owner scalar bounds"
    System 1
End If
If result.items(-2).textValue <> "left" Or result.items(-2).numberValue <> 20 Then
    Print "FAIL owner scalar item -2"
    System 1
End If
If result.items(-1).textValue <> "mid" + Chr$(0) + "nul" Or result.items(-1).numberValue <> 21 Then
    Print "FAIL owner scalar item -1"
    System 1
End If
If result.items(0).textValue <> "right" Or result.items(0).numberValue <> 22 Then
    Print "FAIL owner scalar item 0"
    System 1
End If

Print "PASS Func_return_UDT_t059"
System 0

Function MakeDynOwnerScalar As DynOwnerScalarResultData
    ReDim MakeDynOwnerScalar.items(-2 To 0)
    MakeDynOwnerScalar.items(-2).textValue = "left"
    MakeDynOwnerScalar.items(-2).numberValue = 20
    MakeDynOwnerScalar.items(-1).textValue = "mid" + Chr$(0) + "nul"
    MakeDynOwnerScalar.items(-1).numberValue = 21
    MakeDynOwnerScalar.items(0).textValue = "right"
    MakeDynOwnerScalar.items(0).numberValue = 22
End Function
