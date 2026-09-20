$Console:Only
Option _Explicit

Type TextData
    text As String
    code As Long
End Type

Dim result As TextData

' A no-argument FUNCTION call has no parentheses in QB64PE.
result = MakeTextData

If result.text <> "direct scalar STRING" Then
    Print "FAIL scalar variable STRING: ["; result.text; "]"
    System 1
End If
If result.code <> 109 Then
    Print "FAIL scalar variable STRING code:"; result.code
    System 1
End If

Print "PASS Func_return_UDT_t017"
System 0

Function MakeTextData As TextData
    MakeTextData.text = "direct scalar STRING"
    MakeTextData.code = 109
End Function
