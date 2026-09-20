$Console:Only
Option _Explicit

Type LargeOwnerLeafData
    text As String
    value As Long
End Type

Type LargeOwnerArrayData
    items(0 To 8191) As LargeOwnerLeafData
End Type

Dim result As LargeOwnerArrayData

result = MakeLargeOwnerArray

If result.items(0).text <> "first" Or result.items(0).value <> 1 Then
    Print "FAIL large owner UDT array first item"
    System 1
End If
If result.items(4096).text <> "middle" Or result.items(4096).value <> 4096 Then
    Print "FAIL large owner UDT array middle item"
    System 1
End If
If result.items(8191).text <> "last" Or result.items(8191).value <> 8191 Then
    Print "FAIL large owner UDT array last item"
    System 1
End If
If result.items(1).text <> "" Or result.items(1).value <> 0 Then
    Print "FAIL large owner UDT array untouched item"
    System 1
End If

Print "PASS Func_return_UDT_t041"
System 0

Function MakeLargeOwnerArray As LargeOwnerArrayData
    MakeLargeOwnerArray.items(0).text = "first"
    MakeLargeOwnerArray.items(0).value = 1
    MakeLargeOwnerArray.items(4096).text = "middle"
    MakeLargeOwnerArray.items(4096).value = 4096
    MakeLargeOwnerArray.items(8191).text = "last"
    MakeLargeOwnerArray.items(8191).value = 8191
End Function
