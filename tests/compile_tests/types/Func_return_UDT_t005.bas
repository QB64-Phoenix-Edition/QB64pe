$Console:Only
Option _Explicit

Type ResultData
    alwaysSet As Long
    conditionalSet As Long
End Type

Dim result As ResultData
Dim i As Long

For i = 1 To 20
    result = MakeResult(i, -1)
    If result.alwaysSet <> i Or result.conditionalSet <> 999 Then
        Print "FAIL set iteration:"; i; result.alwaysSet; result.conditionalSet
        System 1
    End If

    result = MakeResult(i, 0)
    If result.alwaysSet <> i Then
        Print "FAIL reset value:"; i; result.alwaysSet
        System 1
    End If
    If result.conditionalSet <> 0 Then
        Print "FAIL stale return storage:"; i; result.conditionalSet
        System 1
    End If
Next

Print "PASS Func_return_UDT_t005"
System 0

Function MakeResult (value As Long, setConditional As Integer) As ResultData
    MakeResult.alwaysSet = value
    If setConditional Then MakeResult.conditionalSet = 999
End Function
