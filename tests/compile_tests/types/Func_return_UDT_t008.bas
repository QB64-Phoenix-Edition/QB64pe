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
    If n <= 0 Then
        BuildResult.total = 0
        BuildResult.depth = 0
    Else
        BuildResult = BuildResult(n - 1)
        BuildResult.total = BuildResult.total + n
        BuildResult.depth = BuildResult.depth + 1
    End If
End Function
