$Console:Only
Option _Explicit

Type LocalLeafData
    text As String
    value As Long
End Type

Type LocalRootData
    part As LocalLeafData
    labelText As String
End Type

Dim result As LocalRootData
result = MakeNestedFromLocal("inner local", "outer local", 128)

If result.part.text <> "inner local" Then
    Print "FAIL nested local inner STRING: ["; result.part.text; "]"
    System 1
End If
If result.labelText <> "outer local" Then
    Print "FAIL nested local outer STRING: ["; result.labelText; "]"
    System 1
End If
If result.part.value <> 128 Then
    Print "FAIL nested local value:"; result.part.value
    System 1
End If

Print "PASS Func_return_UDT_t024"
System 0

Function MakeNestedFromLocal (innerText As String, outerText As String, numberValue As Long) As LocalRootData
    Dim temp As LocalRootData
    temp.part.text = innerText
    temp.part.value = numberValue
    temp.labelText = outerText
    MakeNestedFromLocal = temp
End Function
