$Console:Only
Option _Explicit

Type DeepOwnedLeafData
    text As String
End Type

Type DeepOwnedItemData
    part As DeepOwnedLeafData
    value As Long
End Type

Type DeepOwnedArrayResultData
    items(0 To 2) As DeepOwnedItemData
End Type

Dim result As DeepOwnedArrayResultData

result = MakeDeepOwnedArray

If result.items(0).part.text <> "zero" Or result.items(0).value <> 100 Then
    Print "FAIL deep owner UDT array item 0"
    System 1
End If
If result.items(1).part.text <> "one" Or result.items(1).value <> 101 Then
    Print "FAIL deep owner UDT array item 1"
    System 1
End If
If result.items(2).part.text <> "two" Or result.items(2).value <> 102 Then
    Print "FAIL deep owner UDT array item 2"
    System 1
End If

Print "PASS Func_return_UDT_t035"
System 0

Function MakeDeepOwnedArray As DeepOwnedArrayResultData
    MakeDeepOwnedArray.items(0).part.text = "zero"
    MakeDeepOwnedArray.items(0).value = 100
    MakeDeepOwnedArray.items(1).part.text = "one"
    MakeDeepOwnedArray.items(1).value = 101
    MakeDeepOwnedArray.items(2).part.text = "two"
    MakeDeepOwnedArray.items(2).value = 102
End Function
