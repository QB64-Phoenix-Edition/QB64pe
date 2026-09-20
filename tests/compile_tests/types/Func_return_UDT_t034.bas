$Console:Only
Option _Explicit

Type OwnedLeafData
    text As String
    value As Long
End Type

Type OwnedLeafArrayData
    items(0 To 1) As OwnedLeafData
    marker As Long
End Type

Dim result As OwnedLeafArrayData

result = MakeOwnedLeafArray

If result.items(0).text <> "alpha" Or result.items(0).value <> 10 Then
    Print "FAIL owner UDT array item 0"
    System 1
End If
If result.items(1).text <> "beta" Or result.items(1).value <> 20 Then
    Print "FAIL owner UDT array item 1"
    System 1
End If
If result.marker <> 99 Then
    Print "FAIL owner UDT array marker:"; result.marker
    System 1
End If

Print "PASS Func_return_UDT_t034"
System 0

Function MakeOwnedLeafArray As OwnedLeafArrayData
    MakeOwnedLeafArray.items(0).text = "alpha"
    MakeOwnedLeafArray.items(0).value = 10
    MakeOwnedLeafArray.items(1).text = "beta"
    MakeOwnedLeafArray.items(1).value = 20
    MakeOwnedLeafArray.marker = 99
End Function
