$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicPairData
    x As Long
    y As Long
End Type

Type DynamicPairResultData
    items(0 To 0) _Dynamic As DynamicPairData
End Type

Dim result As DynamicPairResultData

result = MakeDynamicPairs

If LBound(result.items) <> 3 Or UBound(result.items) <> 4 Then
    Print "FAIL dynamic UDT bounds"
    System 1
End If
If result.items(3).x <> 30 Or result.items(3).y <> 31 Then
    Print "FAIL dynamic UDT item 3"
    System 1
End If
If result.items(4).x <> 40 Or result.items(4).y <> 41 Then
    Print "FAIL dynamic UDT item 4"
    System 1
End If

Print "PASS Func_return_UDT_t046"
System 0

Function MakeDynamicPairs As DynamicPairResultData
    ReDim MakeDynamicPairs.items(3 To 4)
    MakeDynamicPairs.items(3).x = 30
    MakeDynamicPairs.items(3).y = 31
    MakeDynamicPairs.items(4).x = 40
    MakeDynamicPairs.items(4).y = 41
End Function
