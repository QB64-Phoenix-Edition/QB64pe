$Console:Only
Option _Explicit

Type StringArrayData
    texts(0 To 2) As String
    code As Long
End Type

Dim result As StringArrayData

result = MakeStringArray

If result.texts(0) <> "zero" Or result.texts(1) <> "one" Or result.texts(2) <> "two" Then
    Print "FAIL inline variable STRING array: ["; result.texts(0); "] ["; result.texts(1); "] ["; result.texts(2); "]"
    System 1
End If
If result.code <> 123 Then
    Print "FAIL inline variable STRING array scalar:"; result.code
    System 1
End If

Print "PASS Func_return_UDT_t028"
System 0

Function MakeStringArray As StringArrayData
    Dim functionResult As StringArrayData

    functionResult.texts(0) = "zero"
    functionResult.texts(1) = "one"
    functionResult.texts(2) = "two"
    functionResult.code = 123
    MakeStringArray = functionResult
End Function
