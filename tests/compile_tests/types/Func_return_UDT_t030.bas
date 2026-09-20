$Console:Only
Option _Explicit

Type NestedStringArrayLeaf
    names(-1 To 1) As String
    marker As Long
End Type

Type NestedStringArrayRoot
    part As NestedStringArrayLeaf
    outerText As String
End Type

Dim result As NestedStringArrayRoot

result = MakeNestedStringArray

If result.part.names(-1) <> "minus" Or result.part.names(0) <> "zero" Or result.part.names(1) <> "plus" Then
    Print "FAIL nested variable STRING array"
    System 1
End If
If result.part.marker <> 77 Or result.outerText <> "outer" Then
    Print "FAIL nested variable STRING companion members"
    System 1
End If

Print "PASS Func_return_UDT_t030"
System 0

Function MakeNestedStringArray As NestedStringArrayRoot
    MakeNestedStringArray.part.names(-1) = "minus"
    MakeNestedStringArray.part.names(0) = "zero"
    MakeNestedStringArray.part.names(1) = "plus"
    MakeNestedStringArray.part.marker = 77
    MakeNestedStringArray.outerText = "outer"
End Function
