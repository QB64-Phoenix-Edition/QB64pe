$Console:Only
Option _Explicit

Type TextData
    text As String
    code As Long
End Type

Dim result As TextData
Dim i As Long

' The same textual call site executes twice.  The second invocation deliberately does
' not assign the STRING member; caller-owned return storage must not retain the old text.
For i = 1 To 2
    result = MaybeText(i = 1)
    If i = 1 Then
        If result.text <> "first value" Or result.code <> 1 Then
            Print "FAIL first call: ["; result.text; "]"; result.code
            System 1
        End If
    Else
        If result.text <> "" Or result.code <> 2 Then
            Print "FAIL reused call-site reset: ["; result.text; "]"; result.code
            System 1
        End If
    End If
Next

Print "PASS Func_return_UDT_t019"
System 0

Function MaybeText (writeText As Integer) As TextData
    Dim functionResult As TextData

    If writeText Then
        functionResult.text = "first value"
        functionResult.code = 1
    Else
        functionResult.code = 2
    End If
    MaybeText = functionResult
End Function
