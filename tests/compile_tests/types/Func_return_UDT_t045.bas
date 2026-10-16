$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicFixedStringResultData
    labels(1 To 2) _Dynamic As String * 6
End Type

Dim result As DynamicFixedStringResultData

result = MakeDynamicFixedString

If LBound(result.labels) <> -1 Or UBound(result.labels) <> 1 Then
    Print "FAIL fixed STRING dynamic bounds"
    System 1
End If
If RTrim$(result.labels(-1)) <> "red" Or RTrim$(result.labels(0)) <> "green" Or RTrim$(result.labels(1)) <> "blue" Then
    Print "FAIL fixed STRING dynamic payload"
    System 1
End If

Print "PASS Func_return_UDT_t045"
System 0

Function MakeDynamicFixedString As DynamicFixedStringResultData
    Dim functionResult As DynamicFixedStringResultData

    ReDim functionResult.labels(-1 To 1)
    functionResult.labels(-1) = "red"
    functionResult.labels(0) = "green"
    functionResult.labels(1) = "blue"
    MakeDynamicFixedString = functionResult
End Function
