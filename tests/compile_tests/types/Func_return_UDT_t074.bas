$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicExpressionItemData
    textValue As String
    numberValue As Long
End Type

Type DynamicExpressionOwnerData
    texts(0 To 0) _Dynamic As String
    items(0 To 0) _Dynamic As DynamicExpressionItemData
End Type

Dim textValue As String
Dim nestedTextValue As String
Dim numberValue As Long

textValue = MakeDynamicExpressionOwner(740).texts(0)
nestedTextValue = MakeDynamicExpressionOwner(741).items(2).textValue
numberValue = MakeDynamicExpressionOwner(742).items(2).numberValue

If textValue <> "text-740" Then
    Print "FAIL direct dynamic variable STRING member"
    System 1
End If
If nestedTextValue <> "item-741" Then
    Print "FAIL direct dynamic owner nested STRING member"
    System 1
End If
If numberValue <> 742 Then
    Print "FAIL direct dynamic owner nested numeric member"
    System 1
End If

Print "PASS Func_return_UDT_t074"
System 0

Function MakeDynamicExpressionOwner (seed As Long) As DynamicExpressionOwnerData
    Dim functionResult As DynamicExpressionOwnerData

    ReDim functionResult.texts(-1 To 0)
    functionResult.texts(-1) = "prefix"
    functionResult.texts(0) = "text-" + _Trim$(Str$(seed))
    ReDim functionResult.items(1 To 2)
    functionResult.items(1).textValue = "first"
    functionResult.items(1).numberValue = seed - 1
    functionResult.items(2).textValue = "item-" + _Trim$(Str$(seed))
    functionResult.items(2).numberValue = seed
    MakeDynamicExpressionOwner = functionResult
End Function
