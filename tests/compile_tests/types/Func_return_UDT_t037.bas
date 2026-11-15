$Console:Only
Option _Explicit

Type LocalOwnerLeafData
    text As String
    value As Long
End Type

Type LocalOwnerArrayData
    items(0 To 2) As LocalOwnerLeafData
End Type

Dim result As LocalOwnerArrayData

result = MakeLocalOwnerArray("copy")

If result.items(0).text <> "copy-0" Or result.items(0).value <> 40 Then
    Print "FAIL local owner UDT array item 0"
    System 1
End If
If result.items(1).text <> "copy-1" Or result.items(1).value <> 41 Then
    Print "FAIL local owner UDT array item 1"
    System 1
End If
If result.items(2).text <> "copy-2" Or result.items(2).value <> 42 Then
    Print "FAIL local owner UDT array item 2"
    System 1
End If

Print "PASS Func_return_UDT_t037"
System 0

Function MakeLocalOwnerArray (prefix As String) As LocalOwnerArrayData
    Dim functionResult As LocalOwnerArrayData

    Dim temp As LocalOwnerArrayData

    temp.items(0).text = prefix + "-0"
    temp.items(0).value = 40
    temp.items(1).text = prefix + "-1"
    temp.items(1).value = 41
    temp.items(2).text = prefix + "-2"
    temp.items(2).value = 42
    functionResult = temp

    ' The returned element graph must already own independent qbs copies.
    temp.items(0).text = "changed"
    temp.items(1).text = "changed"
    temp.items(2).text = "changed"
    MakeLocalOwnerArray = functionResult
End Function
