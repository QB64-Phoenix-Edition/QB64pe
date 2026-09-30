$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerRecursiveElemData
    textValue As String
End Type

Type DynOwnerRecursiveResultData
    items(0 To 0) _Dynamic As DynOwnerRecursiveElemData
    depthValue As Long
End Type

Dim result As DynOwnerRecursiveResultData

result = MakeDynOwnerRecursive(4)

If LBound(result.items) <> 0 Or UBound(result.items) <> 0 Then
    Print "FAIL owner recursive bounds"
    System 1
End If
If result.items(0).textValue <> "basexxxx" Or result.depthValue <> 4 Then
    Print "FAIL owner recursive payload"
    System 1
End If

Print "PASS Func_return_UDT_t066"
System 0

Function MakeDynOwnerRecursive (level As Long) As DynOwnerRecursiveResultData
    Dim functionResult As DynOwnerRecursiveResultData

    Dim child As DynOwnerRecursiveResultData

    If level = 0 Then
        functionResult.items(0).textValue = "base"
        functionResult.depthValue = 0
    Else
        child = MakeDynOwnerRecursive(level - 1)
        functionResult = child
        functionResult.items(0).textValue = functionResult.items(0).textValue + "x"
        functionResult.depthValue = level
    End If
    MakeDynOwnerRecursive = functionResult
End Function
