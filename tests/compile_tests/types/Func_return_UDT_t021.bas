$Console:Only
Option _Explicit

Type TextData
    text As String
    depth As Long
End Type

Dim result As TextData
result = BuildText(4)

If result.text <> "ABCD" Then
    Print "FAIL recursive STRING: ["; result.text; "]"
    System 1
End If
If result.depth <> 4 Then
    Print "FAIL recursive depth:"; result.depth
    System 1
End If

Print "PASS Func_return_UDT_t021"
System 0

Function BuildText (n As Long) As TextData
    If n <= 0 Then
        BuildText.text = ""
        BuildText.depth = 0
    Else
        BuildText = BuildText(n - 1)
        BuildText.text = BuildText.text + Chr$(64 + n)
        BuildText.depth = BuildText.depth + 1
    End If
End Function
