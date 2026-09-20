$Console:Only
$Unstable:TypeFields
Option _Explicit

Type StaticStringArrayData
    texts(1 To 3) _Static As String
    value As Long
End Type

Dim result As StaticStringArrayData

result = MakeStaticStrings

If result.texts(1) <> "alpha" Or result.texts(2) <> "beta" Or result.texts(3) <> "gamma" Then
    Print "FAIL _Static variable STRING array"
    System 1
End If
If result.value <> 44 Then
    Print "FAIL _Static variable STRING scalar:"; result.value
    System 1
End If

Print "PASS Func_return_UDT_t029"
System 0

Function MakeStaticStrings As StaticStringArrayData
    Dim temp As StaticStringArrayData

    temp.texts(1) = "alpha"
    temp.texts(2) = "beta"
    temp.texts(3) = "gamma"
    temp.value = 44
    MakeStaticStrings = temp
End Function
