$Console:Only

Type RetainOwnerLeaf
    textValue As String
    numberValue As Long
End Type

Type RetainOwnerRoot
    leaves(0 To 8191) As RetainOwnerLeaf
    tailValue As Long
End Type

ReDim roots(-1 To 1, 2 To 3) As RetainOwnerRoot

roots(0, 2).leaves(0).textValue = "zero"
roots(0, 2).leaves(0).numberValue = 100
roots(0, 2).leaves(4095).textValue = "middle"
roots(0, 2).leaves(4095).numberValue = 200
roots(0, 2).leaves(8191).textValue = "last"
roots(0, 2).leaves(8191).numberValue = 300
roots(0, 2).tailValue = 777

roots(1, 2).leaves(123).textValue = "stay coordinate"
roots(1, 2).leaves(123).numberValue = 456
roots(1, 3).leaves(123).textValue = "drop coordinate"
roots(1, 3).leaves(123).numberValue = 999

ReDim _Retain roots(0 To 2, 1 To 2) As RetainOwnerRoot

If LBound(roots, 1) <> 0 Or UBound(roots, 1) <> 2 Then Print "FAIL first bounds": System 1
If LBound(roots, 2) <> 1 Or UBound(roots, 2) <> 2 Then Print "FAIL second bounds": System 1

If roots(0, 2).leaves(0).textValue <> "zero" Then Print "FAIL retain first": System 1
If roots(0, 2).leaves(0).numberValue <> 100 Then Print "FAIL retain first number": System 1
If roots(0, 2).leaves(4095).textValue <> "middle" Then Print "FAIL retain middle": System 1
If roots(0, 2).leaves(4095).numberValue <> 200 Then Print "FAIL retain middle number": System 1
If roots(0, 2).leaves(8191).textValue <> "last" Then Print "FAIL retain last": System 1
If roots(0, 2).leaves(8191).numberValue <> 300 Then Print "FAIL retain last number": System 1
If roots(0, 2).tailValue <> 777 Then Print "FAIL retain tail": System 1

If roots(1, 2).leaves(123).textValue <> "stay coordinate" Then Print "FAIL second retained coordinate": System 1
If roots(1, 2).leaves(123).numberValue <> 456 Then Print "FAIL second retained number": System 1

Print "PASS t_qb64pe_refactor_retain_nested_owner_array"
System
