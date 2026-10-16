$Console:Only
Option _Explicit

Type NoArgumentExpressionData
    numberValue As Long
    textValue As String
End Type

Dim Shared makeCalls As Long
Dim numberValue As Long
Dim textValue As String

numberValue = MakeNoArgumentExpression.numberValue
textValue = MakeNoArgumentExpression.textValue

If numberValue <> 750 Then
    Print "FAIL no-argument direct numeric member"
    System 1
End If
If textValue <> "no arguments" Then
    Print "FAIL no-argument direct STRING member"
    System 1
End If
If makeCalls <> 2 Then
    Print "FAIL no-argument call count:"; makeCalls
    System 1
End If

Print "PASS Func_return_UDT_t075"
System 0

Function MakeNoArgumentExpression As NoArgumentExpressionData
    Dim functionResult As NoArgumentExpressionData

    makeCalls = makeCalls + 1
    functionResult.numberValue = 750
    functionResult.textValue = "no arguments"
    MakeNoArgumentExpression = functionResult
End Function
