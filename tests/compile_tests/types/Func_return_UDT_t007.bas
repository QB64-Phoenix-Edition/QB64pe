$Console:Only
Option _Explicit

Type ResultData
    a As Long
    b As Double
End Type

Dim result As ResultData
result = MakeNoArgs

If result.a <> 456 Then
    Print "FAIL no-args a:"; result.a
    System 1
End If
If Abs(result.b - 78.25) > .0000001 Then
    Print "FAIL no-args b:"; result.b
    System 1
End If

Print "PASS Func_return_UDT_t007"
System 0

Function MakeNoArgs As ResultData
    Dim functionResult As ResultData

    functionResult.a = 456
    functionResult.b = 78.25
    MakeNoArgs = functionResult
End Function
