$Console:Only
Option _Explicit

Type LeafData
    value As Long
    code As String * 3
End Type

Type LeafArrayData
    items(0 To 2) As LeafData
    marker As Integer
End Type

Dim result As LeafArrayData
Dim i As Long

result = MakeLeafArray(30)

For i = 0 To 2
    If result.items(i).value <> 30 + i Then
        Print "FAIL UDT array value:"; i; result.items(i).value
        System 1
    End If
Next
If result.items(0).code <> "A0 " Then Print "FAIL code 0: ["; result.items(0).code; "]": System 1
If result.items(1).code <> "A1 " Then Print "FAIL code 1: ["; result.items(1).code; "]": System 1
If result.items(2).code <> "A2 " Then Print "FAIL code 2: ["; result.items(2).code; "]": System 1
If result.marker <> -123 Then Print "FAIL marker:"; result.marker: System 1

Print "PASS Func_return_UDT_t016"
System 0

Function MakeLeafArray (seed As Long) As LeafArrayData
    Dim temp As LeafArrayData

    temp.items(0).value = seed
    temp.items(0).code = "A0"
    temp.items(1).value = seed + 1
    temp.items(1).code = "A1"
    temp.items(2).value = seed + 2
    temp.items(2).code = "A2"
    temp.marker = -123
    MakeLeafArray = temp
End Function
