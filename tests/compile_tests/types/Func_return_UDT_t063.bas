$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerDescElemData
    nameText As String
    texts(0 To 0) _Dynamic As String
End Type

Type DynOwnerDescResultData
    items(0 To 0) _Dynamic As DynOwnerDescElemData
End Type

Dim result As DynOwnerDescResultData

result = MakeDynOwnerDesc

If LBound(result.items) <> 4 Or UBound(result.items) <> 5 Then
    Print "FAIL nested descriptor outer bounds"
    System 1
End If
If result.items(4).nameText <> "first" Or result.items(5).nameText <> "second" Then
    Print "FAIL nested descriptor scalar strings"
    System 1
End If
If LBound(result.items(4).texts) <> -1 Or UBound(result.items(4).texts) <> 1 Then
    Print "FAIL nested descriptor first bounds"
    System 1
End If
If LBound(result.items(5).texts) <> 7 Or UBound(result.items(5).texts) <> 8 Then
    Print "FAIL nested descriptor second bounds"
    System 1
End If
If result.items(4).texts(-1) <> "a" Or result.items(4).texts(0) <> "b" Or result.items(4).texts(1) <> "c" Then
    Print "FAIL nested descriptor first payload"
    System 1
End If
If result.items(5).texts(7) <> "seven" Or result.items(5).texts(8) <> "eight" Then
    Print "FAIL nested descriptor second payload"
    System 1
End If

Print "PASS Func_return_UDT_t063"
System 0

Function MakeDynOwnerDesc As DynOwnerDescResultData
    Dim functionResult As DynOwnerDescResultData

    ReDim functionResult.items(4 To 5)
    functionResult.items(4).nameText = "first"
    ReDim functionResult.items(4).texts(-1 To 1)
    functionResult.items(4).texts(-1) = "a"
    functionResult.items(4).texts(0) = "b"
    functionResult.items(4).texts(1) = "c"
    functionResult.items(5).nameText = "second"
    ReDim functionResult.items(5).texts(7 To 8)
    functionResult.items(5).texts(7) = "seven"
    functionResult.items(5).texts(8) = "eight"
    MakeDynOwnerDesc = functionResult
End Function
