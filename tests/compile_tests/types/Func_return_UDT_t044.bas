$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicReuseResultData
    values(0 To 1) _Dynamic As Long
    callNumber As Long
End Type

Dim result As DynamicReuseResultData
Dim i As Long

For i = 1 To 2
    result = MakeDynamicReuse(i = 1)
    If i = 1 Then
        If LBound(result.values) <> 5 Or UBound(result.values) <> 6 Then
            Print "FAIL first reuse bounds"
            System 1
        End If
        If result.values(5) <> 50 Or result.values(6) <> 60 Or result.callNumber <> 1 Then
            Print "FAIL first reuse payload"
            System 1
        End If
    Else
        ' The reused hidden slot must have freed the old descriptor and recreated
        ' the declaration bounds before the second FUNCTION invocation.
        If LBound(result.values) <> 0 Or UBound(result.values) <> 1 Then
            Print "FAIL reused descriptor did not reset bounds"
            System 1
        End If
        If result.values(0) <> 0 Or result.values(1) <> 0 Or result.callNumber <> 2 Then
            Print "FAIL reused descriptor did not reset payload"
            System 1
        End If
    End If
Next

Print "PASS Func_return_UDT_t044"
System 0

Function MakeDynamicReuse (writePayload As Integer) As DynamicReuseResultData
    If writePayload Then
        ReDim MakeDynamicReuse.values(5 To 6)
        MakeDynamicReuse.values(5) = 50
        MakeDynamicReuse.values(6) = 60
        MakeDynamicReuse.callNumber = 1
    Else
        MakeDynamicReuse.callNumber = 2
    End If
End Function
