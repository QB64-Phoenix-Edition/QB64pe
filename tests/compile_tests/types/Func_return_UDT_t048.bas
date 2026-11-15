$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicMixedOwnerResultData
    title As String
    values(0 To 1) _Dynamic As Long
End Type

Dim result As DynamicMixedOwnerResultData

result = MakeDynamicMixedOwner("alpha")

If result.title <> "alpha" Then
    Print "FAIL mixed owner title"
    System 1
End If
If LBound(result.values) <> 4 Or UBound(result.values) <> 5 Then
    Print "FAIL mixed owner dynamic bounds"
    System 1
End If
If result.values(4) <> 404 Or result.values(5) <> 405 Then
    Print "FAIL mixed owner dynamic payload"
    System 1
End If

Print "PASS Func_return_UDT_t048"
System 0

Function MakeDynamicMixedOwner (textValue As String) As DynamicMixedOwnerResultData
    Dim functionResult As DynamicMixedOwnerResultData

    functionResult.title = textValue
    ReDim functionResult.values(4 To 5)
    functionResult.values(4) = 404
    functionResult.values(5) = 405
    MakeDynamicMixedOwner = functionResult
End Function
