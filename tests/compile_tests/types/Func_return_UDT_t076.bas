$Console:Only
Option _Explicit

Type CountedExpressionData
    value As Long
End Type

Dim Shared makeCalls As Long
Dim resultValue As Long

resultValue = MakeCountedExpression(10).value + MakeCountedExpression(20).value
If resultValue <> 30 Or makeCalls <> 2 Then
    Print "FAIL arithmetic direct member evaluation:"; resultValue; makeCalls
    System 1
End If

resultValue = EchoLong(MakeCountedExpression(30).value)
If resultValue <> 30 Or makeCalls <> 3 Then
    Print "FAIL direct member function argument:"; resultValue; makeCalls
    System 1
End If

If MakeCountedExpression(40).value <> 40 Then
    Print "FAIL direct member comparison"
    System 1
End If
If makeCalls <> 4 Then
    Print "FAIL counted expression evaluated more than once:"; makeCalls
    System 1
End If

Print "PASS Func_return_UDT_t076"
System 0

Function MakeCountedExpression (value As Long) As CountedExpressionData
    Dim functionResult As CountedExpressionData

    makeCalls = makeCalls + 1
    functionResult.value = value
    MakeCountedExpression = functionResult
End Function

Function EchoLong& (value As Long)
    EchoLong = value
End Function
