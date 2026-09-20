$Console:Only
Option _Explicit

Type FixedData
    code As String * 8
    number As Long
End Type

Dim result As FixedData

result = MakeFixed("QB64", 77)
If result.code <> "QB64    " Then
    Print "FAIL fixed string: ["; result.code; "]"
    System 1
End If
If result.number <> 77 Then
    Print "FAIL fixed number:"; result.number
    System 1
End If

Print "PASS Func_return_UDT_t003"
System 0

Function MakeFixed (text As String, number As Long) As FixedData
    MakeFixed.code = text
    MakeFixed.number = number
End Function
