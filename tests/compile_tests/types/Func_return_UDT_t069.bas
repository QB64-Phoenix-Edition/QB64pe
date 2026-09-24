$Console:Only
$Unstable:TypeFields
Option _Explicit

Type Ref2LeafData
    textValue As String
End Type

Type Ref2BranchData
    leaves(0 To 0) _Dynamic As Ref2LeafData
    branchText As String
End Type

Type Ref2ResultData
    branches(0 To 0) _Dynamic As Ref2BranchData
End Type

Dim result As Ref2ResultData

result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result
result = MakeRef2Result

If LBound(result.branches) <> 2 Or UBound(result.branches) <> 2 Then
    Print "FAIL ref2 outer bounds"
    System 1
End If
If result.branches(2).branchText <> "branch" Then
    Print "FAIL ref2 branch text"
    System 1
End If
If LBound(result.branches(2).leaves) <> -1 Or UBound(result.branches(2).leaves) <> 0 Then
    Print "FAIL ref2 leaf bounds"
    System 1
End If
If result.branches(2).leaves(-1).textValue <> "left" Or result.branches(2).leaves(0).textValue <> "right" Then
    Print "FAIL ref2 leaf payload"
    System 1
End If

Print "PASS Func_return_UDT_t069"
System 0

Function MakeRef2Result As Ref2ResultData
    Dim functionResult As Ref2ResultData

    ReDim functionResult.branches(2 To 2)
    functionResult.branches(2).branchText = "branch"
    ReDim functionResult.branches(2).leaves(-1 To 0)
    functionResult.branches(2).leaves(-1).textValue = "left"
    functionResult.branches(2).leaves(0).textValue = "right"
    MakeRef2Result = functionResult
End Function
