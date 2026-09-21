$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynVarArrayLeafData
    values(0 To 0) _Dynamic As String
End Type

Type DynVarInlineArrayData
    items(0 To 1) As DynVarArrayLeafData
End Type

Dim result As DynVarInlineArrayData

result = MakeDynVarInlineArray

If LBound(result.items(0).values) <> 2 Or UBound(result.items(0).values) <> 3 Then
    Print "FAIL inline item 0 bounds"
    System 1
End If
If LBound(result.items(1).values) <> 4 Or UBound(result.items(1).values) <> 5 Then
    Print "FAIL inline item 1 bounds"
    System 1
End If
If result.items(0).values(2) <> "two" Or result.items(0).values(3) <> "three" Then
    Print "FAIL inline item 0 payload"
    System 1
End If
If result.items(1).values(4) <> "four" Or result.items(1).values(5) <> "five" Then
    Print "FAIL inline item 1 payload"
    System 1
End If

Print "PASS Func_return_UDT_t055"
System 0

Function MakeDynVarInlineArray As DynVarInlineArrayData
    Dim functionResult As DynVarInlineArrayData

    ReDim functionResult.items(0).values(2 To 3)
    ReDim functionResult.items(1).values(4 To 5)
    functionResult.items(0).values(2) = "two"
    functionResult.items(0).values(3) = "three"
    functionResult.items(1).values(4) = "four"
    functionResult.items(1).values(5) = "five"
    MakeDynVarInlineArray = functionResult
End Function
