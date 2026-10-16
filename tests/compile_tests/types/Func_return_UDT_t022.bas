$Console:Only
Option _Explicit

Type NestedTextData
    text As String
    value As Long
End Type

Type NestedResultData
    payload As NestedTextData
    code As Long
End Type

Dim result As NestedResultData
result = MakeNestedText("nested scalar owner", 117)

If result.payload.text <> "nested scalar owner" Then
    Print "FAIL nested STRING: ["; result.payload.text; "]"
    System 1
End If
If result.payload.value <> 117 Or result.code <> 234 Then
    Print "FAIL nested values:"; result.payload.value; result.code
    System 1
End If

Print "PASS Func_return_UDT_t022"
System 0

Function MakeNestedText (valueText As String, numberValue As Long) As NestedResultData
    Dim functionResult As NestedResultData

    functionResult.payload.text = valueText
    functionResult.payload.value = numberValue
    functionResult.code = numberValue * 2
    MakeNestedText = functionResult
End Function
