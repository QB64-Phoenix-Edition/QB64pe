$Console:Only
$Unstable:TypeFields
Option _Explicit

Type NestedDynamicLeafData
    values(0 To 1) _Dynamic As Long
End Type

Type NestedDynamicResultData
    leaf As NestedDynamicLeafData
    marker As Long
End Type

Dim result As NestedDynamicResultData

result = MakeNestedDynamic

If LBound(result.leaf.values) <> 10 Or UBound(result.leaf.values) <> 12 Then
    Print "FAIL nested dynamic bounds"
    System 1
End If
If result.leaf.values(10) <> 110 Or result.leaf.values(11) <> 111 Or result.leaf.values(12) <> 112 Then
    Print "FAIL nested dynamic payload"
    System 1
End If
If result.marker <> 150 Then
    Print "FAIL nested dynamic marker"
    System 1
End If

Print "PASS Func_return_UDT_t047"
System 0

Function MakeNestedDynamic As NestedDynamicResultData
    ReDim MakeNestedDynamic.leaf.values(10 To 12)
    MakeNestedDynamic.leaf.values(10) = 110
    MakeNestedDynamic.leaf.values(11) = 111
    MakeNestedDynamic.leaf.values(12) = 112
    MakeNestedDynamic.marker = 150
End Function
