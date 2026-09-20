$Console:Only
$Unstable:TypeFields
Option _Explicit

Type StaticOwnerLeafData
    text As String
    value As Long
End Type

Type StaticOwnerArrayData
    items(1 To 3) _Static As StaticOwnerLeafData
End Type

Dim result As StaticOwnerArrayData

result = MakeStaticOwnerArray

If result.items(1).text <> "red" Or result.items(1).value <> 1 Then
    Print "FAIL _Static owner UDT array item 1"
    System 1
End If
If result.items(2).text <> "green" Or result.items(2).value <> 2 Then
    Print "FAIL _Static owner UDT array item 2"
    System 1
End If
If result.items(3).text <> "blue" Or result.items(3).value <> 3 Then
    Print "FAIL _Static owner UDT array item 3"
    System 1
End If

Print "PASS Func_return_UDT_t036"
System 0

Function MakeStaticOwnerArray As StaticOwnerArrayData
    Dim temp As StaticOwnerArrayData

    temp.items(1).text = "red"
    temp.items(1).value = 1
    temp.items(2).text = "green"
    temp.items(2).value = 2
    temp.items(3).text = "blue"
    temp.items(3).value = 3
    MakeStaticOwnerArray = temp
End Function
