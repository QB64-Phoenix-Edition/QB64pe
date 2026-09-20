$Console:Only
Option _Explicit

Type RecursiveLeafData
    text As String
End Type

Type RecursiveRootData
    part As RecursiveLeafData
    depth As Long
End Type

Dim result As RecursiveRootData
result = BuildNestedText(4)

If result.part.text <> "ABCD" Then
    Print "FAIL nested recursive STRING: ["; result.part.text; "]"
    System 1
End If
If result.depth <> 4 Then
    Print "FAIL nested recursive depth:"; result.depth
    System 1
End If

Print "PASS Func_return_UDT_t026"
System 0

Function BuildNestedText (n As Long) As RecursiveRootData
    If n <= 0 Then
        BuildNestedText.part.text = ""
        BuildNestedText.depth = 0
    Else
        BuildNestedText = BuildNestedText(n - 1)
        BuildNestedText.part.text = BuildNestedText.part.text + Chr$(64 + n)
        BuildNestedText.depth = BuildNestedText.depth + 1
    End If
End Function
