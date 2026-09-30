$Console:Only
Option _Explicit

Type PairData
    a As Long
    b As Long
End Type

Dim result As PairData

result = MakeShifted(10, 20, 5)
If result.a <> 15 Or result.b <> 25 Then
    Print "FAIL nested call:"; result.a; result.b
    System 1
End If

Print "PASS Func_return_UDT_t004"
System 0

Function MakeBase (a As Long, b As Long) As PairData
    Dim functionResult As PairData

    functionResult.a = a
    functionResult.b = b
    MakeBase = functionResult
End Function

Function MakeShifted (a As Long, b As Long, amount As Long) As PairData
    Dim functionResult As PairData

    functionResult = MakeBase(a, b)
    functionResult.a = functionResult.a + amount
    functionResult.b = functionResult.b + amount
    MakeShifted = functionResult
End Function
