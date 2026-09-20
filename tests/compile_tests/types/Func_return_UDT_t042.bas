$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicLongResultData
    values(0 To 1) _Dynamic As Long
    marker As Long
End Type

Dim result As DynamicLongResultData

result = MakeDynamicLong(100)

If LBound(result.values) <> 2 Or UBound(result.values) <> 4 Then
    Print "FAIL dynamic LONG bounds"
    System 1
End If
If result.values(2) <> 102 Or result.values(3) <> 103 Or result.values(4) <> 104 Then
    Print "FAIL dynamic LONG payload"
    System 1
End If
If result.marker <> 777 Then
    Print "FAIL dynamic LONG marker"
    System 1
End If

Print "PASS Func_return_UDT_t042"
System 0

Function MakeDynamicLong (baseValue As Long) As DynamicLongResultData
    ReDim MakeDynamicLong.values(2 To 4)
    MakeDynamicLong.values(2) = baseValue + 2
    MakeDynamicLong.values(3) = baseValue + 3
    MakeDynamicLong.values(4) = baseValue + 4
    MakeDynamicLong.marker = 777
End Function
