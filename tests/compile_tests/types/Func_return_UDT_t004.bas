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
    MakeBase.a = a
    MakeBase.b = b
End Function

Function MakeShifted (a As Long, b As Long, amount As Long) As PairData
    MakeShifted = MakeBase(a, b)
    MakeShifted.a = MakeShifted.a + amount
    MakeShifted.b = MakeShifted.b + amount
End Function
