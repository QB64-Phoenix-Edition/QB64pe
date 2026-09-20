$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynamicOwnerLeafData
    text As String
    values(0 To 1) _Dynamic As Long
End Type

Type DynamicOwnerArrayResultData
    items(0 To 1) As DynamicOwnerLeafData
End Type

Dim result As DynamicOwnerArrayResultData

result = MakeDynamicOwnerArray

If result.items(0).text <> "zero" Or result.items(1).text <> "one" Then
    Print "FAIL owner array strings"
    System 1
End If
If LBound(result.items(0).values) <> 2 Or UBound(result.items(0).values) <> 3 Then
    Print "FAIL owner array dynamic bounds item 0"
    System 1
End If
If LBound(result.items(1).values) <> 4 Or UBound(result.items(1).values) <> 5 Then
    Print "FAIL owner array dynamic bounds item 1"
    System 1
End If
If result.items(0).values(2) <> 20 Or result.items(0).values(3) <> 30 Then
    Print "FAIL owner array dynamic payload item 0"
    System 1
End If
If result.items(1).values(4) <> 40 Or result.items(1).values(5) <> 50 Then
    Print "FAIL owner array dynamic payload item 1"
    System 1
End If

Print "PASS Func_return_UDT_t049"
System 0

Function MakeDynamicOwnerArray As DynamicOwnerArrayResultData
    MakeDynamicOwnerArray.items(0).text = "zero"
    MakeDynamicOwnerArray.items(1).text = "one"
    ReDim MakeDynamicOwnerArray.items(0).values(2 To 3)
    ReDim MakeDynamicOwnerArray.items(1).values(4 To 5)
    MakeDynamicOwnerArray.items(0).values(2) = 20
    MakeDynamicOwnerArray.items(0).values(3) = 30
    MakeDynamicOwnerArray.items(1).values(4) = 40
    MakeDynamicOwnerArray.items(1).values(5) = 50
End Function
