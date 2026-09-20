$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerMixedElemData
    textValue As String
    numberValue As Long
End Type

Type DynOwnerMixedResultData
    titleText As String
    labels(0 To 0) _Dynamic As String
    items(0 To 0) _Dynamic As DynOwnerMixedElemData
    tailText As String
End Type

Dim result As DynOwnerMixedResultData

result = MakeDynOwnerMixed

If result.titleText <> "title" Or result.tailText <> "tail" Then
    Print "FAIL mixed descriptor scalar strings"
    System 1
End If
If LBound(result.labels) <> 8 Or UBound(result.labels) <> 9 Then
    Print "FAIL mixed descriptor label bounds"
    System 1
End If
If result.labels(8) <> "eight" Or result.labels(9) <> "nine" Then
    Print "FAIL mixed descriptor labels"
    System 1
End If
If LBound(result.items) <> -1 Or UBound(result.items) <> 1 Then
    Print "FAIL mixed descriptor owner bounds"
    System 1
End If
If result.items(-1).textValue <> "minus" Or result.items(-1).numberValue <> -1 Then
    Print "FAIL mixed descriptor owner -1"
    System 1
End If
If result.items(0).textValue <> "zero" Or result.items(0).numberValue <> 0 Then
    Print "FAIL mixed descriptor owner 0"
    System 1
End If
If result.items(1).textValue <> "plus" Or result.items(1).numberValue <> 1 Then
    Print "FAIL mixed descriptor owner 1"
    System 1
End If

Print "PASS Func_return_UDT_t067"
System 0

Function MakeDynOwnerMixed As DynOwnerMixedResultData
    MakeDynOwnerMixed.titleText = "title"
    ReDim MakeDynOwnerMixed.labels(8 To 9)
    MakeDynOwnerMixed.labels(8) = "eight"
    MakeDynOwnerMixed.labels(9) = "nine"
    ReDim MakeDynOwnerMixed.items(-1 To 1)
    MakeDynOwnerMixed.items(-1).textValue = "minus"
    MakeDynOwnerMixed.items(-1).numberValue = -1
    MakeDynOwnerMixed.items(0).textValue = "zero"
    MakeDynOwnerMixed.items(0).numberValue = 0
    MakeDynOwnerMixed.items(1).textValue = "plus"
    MakeDynOwnerMixed.items(1).numberValue = 1
    MakeDynOwnerMixed.tailText = "tail"
End Function
