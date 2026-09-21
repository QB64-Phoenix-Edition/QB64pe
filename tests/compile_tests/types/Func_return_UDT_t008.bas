$Console:Only
Option _Explicit

Type ResultData
    total As Long
    depth As Long
End Type

Dim result As ResultData
result = BuildResult(8)

If result.total <> 36 Then
    Print "FAIL recursive total:"; result.total
    System 1
End If
If result.depth <> 8 Then
    Print "FAIL recursive depth:"; result.depth
    System 1
End If

Print "PASS Func_return_UDT_t008"
System 0

Function BuildResult (n As Long) As ResultData
    Dim functionResult As ResultData

    If n <= 0 Then
        functionResult.total = 0
        functionResult.depth = 0
    Else
        functionResult = BuildResult(n - 1)
        functionResult.total = functionResult.total + n
        functionResult.depth = functionResult.depth + 1
    End If
    BuildResult = functionResult
End Function
