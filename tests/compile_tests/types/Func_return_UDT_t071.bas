$Console:Only
Option _Explicit

Type NestedExpressionItemData
    textValue As String
    score As Long
End Type

Type NestedExpressionRootData
    item As NestedExpressionItemData
    marker As Long
End Type

Dim Shared makeCalls As Long
Dim scoreValue As Long
Dim textValue As String
Dim itemCopy As NestedExpressionItemData

scoreValue = MakeNestedExpression("alpha", 71).item.score
textValue = MakeNestedExpression("beta", 72).item.textValue
itemCopy = MakeNestedExpression("gamma", 73).item

If scoreValue <> 71 Then
    Print "FAIL direct nested numeric member"
    System 1
End If
If textValue <> "beta" Then
    Print "FAIL direct nested STRING member"
    System 1
End If
If itemCopy.textValue <> "gamma" Or itemCopy.score <> 73 Then
    Print "FAIL direct nested UDT value"
    System 1
End If
If makeCalls <> 3 Then
    Print "FAIL nested call count:"; makeCalls
    System 1
End If

Print "PASS Func_return_UDT_t071"
System 0

Function MakeNestedExpression (textValue As String, score As Long) As NestedExpressionRootData
    Dim functionResult As NestedExpressionRootData

    makeCalls = makeCalls + 1
    functionResult.item.textValue = textValue
    functionResult.item.score = score
    functionResult.marker = score * 2
    MakeNestedExpression = functionResult
End Function
