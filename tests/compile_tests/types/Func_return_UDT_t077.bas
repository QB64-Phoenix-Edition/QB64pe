$Console:Only
Option _Explicit

Type ShortCircuitExpressionData
    flagValue As Integer
End Type

Dim Shared makeCalls As Long

If 0 _AndAlso MakeShortCircuitExpression(-1).flagValue Then
    Print "FAIL impossible _ANDALSO branch"
    System 1
End If
If makeCalls <> 0 Then
    Print "FAIL _ANDALSO evaluated direct member call"
    System 1
End If

If -1 _OrElse MakeShortCircuitExpression(-1).flagValue Then
    ' Expected branch. The right side must remain unevaluated.
Else
    Print "FAIL _ORELSE result"
    System 1
End If
If makeCalls <> 0 Then
    Print "FAIL _ORELSE evaluated direct member call"
    System 1
End If

If -1 _AndAlso MakeShortCircuitExpression(-1).flagValue Then
    ' Expected branch. The right side must be evaluated exactly once.
Else
    Print "FAIL live _ANDALSO result"
    System 1
End If
If makeCalls <> 1 Then
    Print "FAIL live short-circuit call count:"; makeCalls
    System 1
End If

Print "PASS Func_return_UDT_t077"
System 0

Function MakeShortCircuitExpression (flagValue As Integer) As ShortCircuitExpressionData
    Dim functionResult As ShortCircuitExpressionData

    makeCalls = makeCalls + 1
    functionResult.flagValue = flagValue
    MakeShortCircuitExpression = functionResult
End Function
