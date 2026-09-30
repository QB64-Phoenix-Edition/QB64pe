$Console:Only
$Unstable:TypeFields
Option _Explicit

Type DynOwnerDeepLeafData
    textValue As String
End Type

Type DynOwnerDeepBranchData
    leaves(0 To 0) _Dynamic As DynOwnerDeepLeafData
    branchText As String
End Type

Type DynOwnerDeepResultData
    branches(0 To 0) _Dynamic As DynOwnerDeepBranchData
End Type

Dim result As DynOwnerDeepResultData

result = MakeDynOwnerDeep

If LBound(result.branches) <> 2 Or UBound(result.branches) <> 3 Then
    Print "FAIL deep owner outer bounds"
    System 1
End If
If result.branches(2).branchText <> "branch2" Or result.branches(3).branchText <> "branch3" Then
    Print "FAIL deep owner branch strings"
    System 1
End If
If LBound(result.branches(2).leaves) <> -1 Or UBound(result.branches(2).leaves) <> 0 Then
    Print "FAIL deep owner leaves 2 bounds"
    System 1
End If
If LBound(result.branches(3).leaves) <> 5 Or UBound(result.branches(3).leaves) <> 7 Then
    Print "FAIL deep owner leaves 3 bounds"
    System 1
End If
If result.branches(2).leaves(-1).textValue <> "2-left" Or result.branches(2).leaves(0).textValue <> "2-right" Then
    Print "FAIL deep owner leaves 2 payload"
    System 1
End If
If result.branches(3).leaves(5).textValue <> "3-five" Or result.branches(3).leaves(6).textValue <> "3-six" Or result.branches(3).leaves(7).textValue <> "3-seven" Then
    Print "FAIL deep owner leaves 3 payload"
    System 1
End If

Print "PASS Func_return_UDT_t068"
System 0

Function MakeDynOwnerDeep As DynOwnerDeepResultData
    Dim functionResult As DynOwnerDeepResultData

    ReDim functionResult.branches(2 To 3)
    functionResult.branches(2).branchText = "branch2"
    ReDim functionResult.branches(2).leaves(-1 To 0)
    functionResult.branches(2).leaves(-1).textValue = "2-left"
    functionResult.branches(2).leaves(0).textValue = "2-right"
    functionResult.branches(3).branchText = "branch3"
    ReDim functionResult.branches(3).leaves(5 To 7)
    functionResult.branches(3).leaves(5).textValue = "3-five"
    functionResult.branches(3).leaves(6).textValue = "3-six"
    functionResult.branches(3).leaves(7).textValue = "3-seven"
    MakeDynOwnerDeep = functionResult
End Function
