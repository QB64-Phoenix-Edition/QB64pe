$Console:Only
Option _Explicit

Type ReuseStringArrayData
    names(0 To 2) As String
    callNumber As Long
End Type

Dim result As ReuseStringArrayData
Dim i As Long

'Execute one textual call site twice.  The second result deliberately leaves the
'STRING array untouched; every qbs* slot must have been reset to a fresh empty owner.
For i = 1 To 2
    result = MaybeStringArray(i = 1)
    If i = 1 Then
        If result.names(0) <> "first-0" Or result.names(1) <> "first-1" Or result.names(2) <> "first-2" Or result.callNumber <> 1 Then
            Print "FAIL first string-array reuse call"
            System 1
        End If
    Else
        If result.names(0) <> "" Or result.names(1) <> "" Or result.names(2) <> "" Or result.callNumber <> 2 Then
            Print "FAIL string-array reused slot reset: ["; result.names(0); "] ["; result.names(1); "] ["; result.names(2); "]"; result.callNumber
            System 1
        End If
    End If
Next

Print "PASS Func_return_UDT_t032"
System 0

Function MaybeStringArray (writeText As Integer) As ReuseStringArrayData
    If writeText Then
        MaybeStringArray.names(0) = "first-0"
        MaybeStringArray.names(1) = "first-1"
        MaybeStringArray.names(2) = "first-2"
        MaybeStringArray.callNumber = 1
    Else
        MaybeStringArray.callNumber = 2
    End If
End Function
