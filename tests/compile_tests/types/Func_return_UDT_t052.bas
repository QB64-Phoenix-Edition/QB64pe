$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarReuseData
    values(0 To 1) _Dynamic As String
    callNumber As Long
End Type

Dim result As DynVarReuseData
Dim i As Long

For i = 1 To 2
    result = MakeDynVarReuse(i = 1)
    If i = 1 Then
        If LBound(result.values) <> 5 Or UBound(result.values) <> 6 Then
            Print "FAIL first varstring reuse bounds"
            System 1
        End If
        If result.values(5) <> "first" Or result.values(6) <> "payload" Or result.callNumber <> 1 Then
            Print "FAIL first varstring reuse payload"
            System 1
        End If
    Else
        If LBound(result.values) <> 0 Or UBound(result.values) <> 1 Then
            Print "FAIL reused varstring descriptor bounds"
            System 1
        End If
        If result.values(0) <> "" Or result.values(1) <> "" Or result.callNumber <> 2 Then
            Print "FAIL reused varstring descriptor payload"
            System 1
        End If
    End If
Next

Print "PASS Func_return_UDT_t052"
System 0

Function MakeDynVarReuse (writePayload As Integer) As DynVarReuseData
    If writePayload Then
        ReDim MakeDynVarReuse.values(5 To 6)
        MakeDynVarReuse.values(5) = "first"
        MakeDynVarReuse.values(6) = "payload"
        MakeDynVarReuse.callNumber = 1
    Else
        MakeDynVarReuse.callNumber = 2
    End If
End Function
