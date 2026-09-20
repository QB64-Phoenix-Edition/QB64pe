$Console:Only
Option _Explicit

Type DeepLeafData
    leafText As String
    leafValue As Long
End Type

Type DeepBranchData
    leafPart As DeepLeafData
    branchText As String
End Type

Type DeepRootData
    branchPart As DeepBranchData
    rootText As String
    code As Long
End Type

Dim result As DeepRootData
result = MakeDeepOwner("leaf", "branch", "root", 127)

If result.branchPart.leafPart.leafText <> "leaf" Then
    Print "FAIL deep leaf STRING: ["; result.branchPart.leafPart.leafText; "]"
    System 1
End If
If result.branchPart.branchText <> "branch" Then
    Print "FAIL deep branch STRING: ["; result.branchPart.branchText; "]"
    System 1
End If
If result.rootText <> "root" Then
    Print "FAIL root STRING: ["; result.rootText; "]"
    System 1
End If
If result.branchPart.leafPart.leafValue <> 127 Or result.code <> 381 Then
    Print "FAIL deep values:"; result.branchPart.leafPart.leafValue; result.code
    System 1
End If

Print "PASS Func_return_UDT_t023"
System 0

Function MakeDeepOwner (leafValueText As String, branchValueText As String, rootValueText As String, numberValue As Long) As DeepRootData
    MakeDeepOwner.branchPart.leafPart.leafText = leafValueText
    MakeDeepOwner.branchPart.leafPart.leafValue = numberValue
    MakeDeepOwner.branchPart.branchText = branchValueText
    MakeDeepOwner.rootText = rootValueText
    MakeDeepOwner.code = numberValue * 3
End Function
