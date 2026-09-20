$Console:Only
Option _Explicit

Type MultiStringArrayData
    names(0 To 1, 2 To 3) As String
End Type

Dim result As MultiStringArrayData

result = MakeMultiStrings

If result.names(0, 2) <> "02" Or result.names(0, 3) <> "03" Or result.names(1, 2) <> "12" Or result.names(1, 3) <> "13" Then
    Print "FAIL multidimensional variable STRING array"
    System 1
End If

Print "PASS Func_return_UDT_t033"
System 0

Function MakeMultiStrings As MultiStringArrayData
    MakeMultiStrings.names(0, 2) = "02"
    MakeMultiStrings.names(0, 3) = "03"
    MakeMultiStrings.names(1, 2) = "12"
    MakeMultiStrings.names(1, 3) = "13"
End Function
