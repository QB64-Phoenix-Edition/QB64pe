$Console:Only
$Unstable:TypeFields
Option _Explicit

Type ReusedExpressionData
    texts(0 To 0) _Dynamic As String
    marker As Long
End Type

Dim Shared makeCalls As Long
Dim loopIndex As Long
Dim expectedText As String
Dim observedText As String

For loopIndex = 1 To 100
    expectedText = "value-" + _Trim$(Str$(loopIndex))
    observedText = MakeReusedExpression(loopIndex).texts(1)
    If observedText <> expectedText Then
        Print "FAIL reused owned result:"; loopIndex; observedText
        System 1
    End If
Next

If makeCalls <> 100 Then
    Print "FAIL reused call-site count:"; makeCalls
    System 1
End If

Print "PASS Func_return_UDT_t078"
System 0

Function MakeReusedExpression (seed As Long) As ReusedExpressionData
    Dim functionResult As ReusedExpressionData

    makeCalls = makeCalls + 1
    ReDim functionResult.texts(0 To 1)
    functionResult.texts(0) = "prefix"
    functionResult.texts(1) = "value-" + _Trim$(Str$(seed))
    functionResult.marker = seed
    MakeReusedExpression = functionResult
End Function
