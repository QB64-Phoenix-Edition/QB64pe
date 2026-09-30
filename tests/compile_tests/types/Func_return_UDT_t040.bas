$Console:Only
Option _Explicit

Type NestedArrayLeafData
    labels(0 To 1) As String
    value As Long
End Type

Type NestedOwnerArraysData
    groups(0 To 1) As NestedArrayLeafData
    title As String
End Type

Dim result As NestedOwnerArraysData

result = MakeNestedOwnerArrays

If result.groups(0).labels(0) <> "A0" Or result.groups(0).labels(1) <> "A1" Or result.groups(0).value <> 50 Then
    Print "FAIL nested owner arrays group 0"
    System 1
End If
If result.groups(1).labels(0) <> "B0" Or result.groups(1).labels(1) <> "B1" Or result.groups(1).value <> 51 Then
    Print "FAIL nested owner arrays group 1"
    System 1
End If
If result.title <> "nested arrays" Then
    Print "FAIL nested owner arrays title"
    System 1
End If

Print "PASS Func_return_UDT_t040"
System 0

Function MakeNestedOwnerArrays As NestedOwnerArraysData
    Dim functionResult As NestedOwnerArraysData

    functionResult.groups(0).labels(0) = "A0"
    functionResult.groups(0).labels(1) = "A1"
    functionResult.groups(0).value = 50
    functionResult.groups(1).labels(0) = "B0"
    functionResult.groups(1).labels(1) = "B1"
    functionResult.groups(1).value = 51
    functionResult.title = "nested arrays"
    MakeNestedOwnerArrays = functionResult
End Function
