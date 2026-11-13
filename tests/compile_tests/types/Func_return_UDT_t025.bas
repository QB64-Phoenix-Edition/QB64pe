$Console:Only
Option _Explicit

Type ReuseLeafData
    text As String
    value As Long
End Type

Type ReuseRootData
    part As ReuseLeafData
    outerText As String
End Type

Dim result As ReuseRootData
Dim i As Long

' Execute one textual call site twice.  The second result deliberately leaves every
' STRING untouched; nested and root qbs* slots must both be fresh empty owners.
For i = 1 To 2
    result = MaybeNestedText(i = 1)
    If i = 1 Then
        If result.part.text <> "nested first" Or result.outerText <> "outer first" Or result.part.value <> 1 Then
            Print "FAIL first nested reuse call: ["; result.part.text; "] ["; result.outerText; "]"; result.part.value
            System 1
        End If
    Else
        If result.part.text <> "" Or result.outerText <> "" Or result.part.value <> 2 Then
            Print "FAIL nested reused slot reset: ["; result.part.text; "] ["; result.outerText; "]"; result.part.value
            System 1
        End If
    End If
Next

Print "PASS Func_return_UDT_t025"
System 0

Function MaybeNestedText (writeText As Integer) As ReuseRootData
    Dim functionResult As ReuseRootData

    If writeText Then
        functionResult.part.text = "nested first"
        functionResult.outerText = "outer first"
        functionResult.part.value = 1
    Else
        functionResult.part.value = 2
    End If
    MaybeNestedText = functionResult
End Function
