$Console:Only
Option _Explicit

Type LocalStringArrayData
    names(1 To 3) As String
End Type

Dim result As LocalStringArrayData

result = MakeLocalStringArray("copy")

If result.names(1) <> "copy-1" Or result.names(2) <> "copy-2" Or result.names(3) <> "copy-3" Then
    Print "FAIL local variable STRING array deep-copy"
    System 1
End If

Print "PASS Func_return_UDT_t031"
System 0

Function MakeLocalStringArray (prefix As String) As LocalStringArrayData
    Dim temp As LocalStringArrayData

    temp.names(1) = prefix + "-1"
    temp.names(2) = prefix + "-2"
    temp.names(3) = prefix + "-3"
    MakeLocalStringArray = temp

    'Overwrite the local owner after assignment.  The return object must already own copies.
    temp.names(1) = "changed"
    temp.names(2) = "changed"
    temp.names(3) = "changed"
End Function
