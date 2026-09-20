$Console:Only
Option _Explicit

Type TextData
    text As String
    code As Long
End Type

Dim result As TextData
result = MakeFromLocal("local owner", 123)

If result.text <> "local owner" Then
    Print "FAIL local UDT deep copy: ["; result.text; "]"
    System 1
End If
If result.code <> 123 Then
    Print "FAIL local UDT code:"; result.code
    System 1
End If

Print "PASS Func_return_UDT_t018"
System 0

Function MakeFromLocal (value As String, codeValue As Long) As TextData
    Dim temp As TextData
    temp.text = value
    temp.code = codeValue
    MakeFromLocal = temp
End Function
