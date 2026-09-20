$Console:Only
$Unstable:TypeFields
Option _Explicit

Type RetainDescLeaf
    As Long values(0 To 1) _Dynamic
End Type

Type RetainDescRoot
    As RetainDescLeaf leaves(0 To 3)
    marker As Long
End Type

Dim itemIndex As Long
ReDim parentData(-1 To 1, 2 To 3) As RetainDescRoot

For itemIndex = 0 To 3
    parentData(0, 2).leaves(itemIndex).values(0) = 100 + itemIndex
    parentData(0, 2).leaves(itemIndex).values(1) = 200 + itemIndex
    parentData(1, 2).leaves(itemIndex).values(0) = 300 + itemIndex
    parentData(1, 2).leaves(itemIndex).values(1) = 400 + itemIndex
    parentData(1, 3).leaves(itemIndex).values(0) = 900 + itemIndex
Next itemIndex
parentData(0, 2).marker = 12
parentData(1, 2).marker = 34

' The new bounds preserve only coordinates (0..1, 2). Descriptor-backed member
' payloads must be deep-copied through AppendDynUDTDescCopy inside the shared walker.
ReDim _Retain parentData(0 To 2, 1 To 2) As RetainDescRoot

For itemIndex = 0 To 3
    If parentData(0, 2).leaves(itemIndex).values(0) <> 100 + itemIndex Then Print "FAIL first descriptor value 0": System 1
    If parentData(0, 2).leaves(itemIndex).values(1) <> 200 + itemIndex Then Print "FAIL first descriptor value 1": System 1
    If parentData(1, 2).leaves(itemIndex).values(0) <> 300 + itemIndex Then Print "FAIL second descriptor value 0": System 1
    If parentData(1, 2).leaves(itemIndex).values(1) <> 400 + itemIndex Then Print "FAIL second descriptor value 1": System 1
Next itemIndex
If parentData(0, 2).marker <> 12 Then Print "FAIL first marker": System 1
If parentData(1, 2).marker <> 34 Then Print "FAIL second marker": System 1

' Fresh coordinates must own valid freshly initialized descriptors.
For itemIndex = 0 To 3
    If parentData(2, 1).leaves(itemIndex).values(0) <> 0 Then Print "FAIL fresh descriptor value 0": System 1
    If parentData(2, 1).leaves(itemIndex).values(1) <> 0 Then Print "FAIL fresh descriptor value 1": System 1
Next itemIndex

' Retained parent elements must remain independent after deep descriptor copy.
parentData(0, 2).leaves(0).values(0) = 777
If parentData(1, 2).leaves(0).values(0) <> 300 Then Print "FAIL descriptor alias": System 1

Print "PASS t_qb64pe_refactor_retain_descriptor_udt"
System
