$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerInlineChildData
    textValue As String
End Type

Type DynOwnerInlineElemData
    children(0 To 1) As DynOwnerInlineChildData
    tailText As String
End Type

Type DynOwnerInlineResultData
    items(0 To 0) _Dynamic As DynOwnerInlineElemData
End Type

Dim result As DynOwnerInlineResultData

result = MakeDynOwnerInline

If LBound(result.items) <> 2 Or UBound(result.items) <> 3 Then
    Print "FAIL owner inline outer bounds"
    System 1
End If
If result.items(2).children(0).textValue <> "2a" Or result.items(2).children(1).textValue <> "2b" Then
    Print "FAIL owner inline children 2"
    System 1
End If
If result.items(3).children(0).textValue <> "3a" Or result.items(3).children(1).textValue <> "3b" Then
    Print "FAIL owner inline children 3"
    System 1
End If
If result.items(2).tailText <> "tail2" Or result.items(3).tailText <> "tail3" Then
    Print "FAIL owner inline tails"
    System 1
End If

Print "PASS Func_return_UDT_t065"
System 0

Function MakeDynOwnerInline As DynOwnerInlineResultData
    ReDim MakeDynOwnerInline.items(2 To 3)
    MakeDynOwnerInline.items(2).children(0).textValue = "2a"
    MakeDynOwnerInline.items(2).children(1).textValue = "2b"
    MakeDynOwnerInline.items(2).tailText = "tail2"
    MakeDynOwnerInline.items(3).children(0).textValue = "3a"
    MakeDynOwnerInline.items(3).children(1).textValue = "3b"
    MakeDynOwnerInline.items(3).tailText = "tail3"
End Function
