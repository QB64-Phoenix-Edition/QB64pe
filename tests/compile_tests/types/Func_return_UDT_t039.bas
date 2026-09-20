$Console:Only
Option _Explicit

Type MultiOwnerLeafData
    text As String
    value As Long
End Type

Type MultiOwnerArrayData
    items(0 To 1, 2 To 3) As MultiOwnerLeafData
End Type

Dim result As MultiOwnerArrayData

result = MakeMultiOwnerArray

If result.items(0, 2).text <> "02" Or result.items(0, 2).value <> 2 Then
    Print "FAIL multidimensional owner UDT item 0,2"
    System 1
End If
If result.items(0, 3).text <> "03" Or result.items(0, 3).value <> 3 Then
    Print "FAIL multidimensional owner UDT item 0,3"
    System 1
End If
If result.items(1, 2).text <> "12" Or result.items(1, 2).value <> 12 Then
    Print "FAIL multidimensional owner UDT item 1,2"
    System 1
End If
If result.items(1, 3).text <> "13" Or result.items(1, 3).value <> 13 Then
    Print "FAIL multidimensional owner UDT item 1,3"
    System 1
End If

Print "PASS Func_return_UDT_t039"
System 0

Function MakeMultiOwnerArray As MultiOwnerArrayData
    MakeMultiOwnerArray.items(0, 2).text = "02"
    MakeMultiOwnerArray.items(0, 2).value = 2
    MakeMultiOwnerArray.items(0, 3).text = "03"
    MakeMultiOwnerArray.items(0, 3).value = 3
    MakeMultiOwnerArray.items(1, 2).text = "12"
    MakeMultiOwnerArray.items(1, 2).value = 12
    MakeMultiOwnerArray.items(1, 3).text = "13"
    MakeMultiOwnerArray.items(1, 3).value = 13
End Function
