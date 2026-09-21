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
    Dim functionResult As DynOwnerScalarResultData

    ReDim functionResult.items(-2 To 0)
    functionResult.items(-2).textValue = "left"
    functionResult.items(-2).numberValue = 20
    functionResult.items(-1).textValue = "mid" + Chr$(0) + "nul"
    functionResult.items(-1).numberValue = 21
    functionResult.items(0).textValue = "right"
    functionResult.items(0).numberValue = 22
    MakeDynOwnerScalar = functionResult
End Function
