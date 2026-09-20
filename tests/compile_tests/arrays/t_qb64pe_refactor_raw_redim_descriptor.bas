$Console:Only
$Unstable:TypeFields
Option _Explicit

Type RawDescLeaf
    As Long values(0 To 1) _Dynamic
End Type

Type RawDescRoot
    As RawDescLeaf leaves(0 To 2)
    marker As Long
End Type

Dim itemIndex As Long
ReDim rootItems(0 To 1) As RawDescRoot

For itemIndex = 0 To 2
    rootItems(0).leaves(itemIndex).values(0) = 100 + itemIndex
    rootItems(0).leaves(itemIndex).values(1) = 200 + itemIndex
    rootItems(1).leaves(itemIndex).values(0) = 300 + itemIndex
    rootItems(1).leaves(itemIndex).values(1) = 400 + itemIndex
Next itemIndex
rootItems(0).marker = 11
rootItems(1).marker = 22

' _PRESERVE uses raw fixed-payload preservation plus descriptor init/deep-copy/free.
ReDim _Preserve rootItems(0 To 3) As RawDescRoot
For itemIndex = 0 To 2
    If rootItems(0).leaves(itemIndex).values(0) <> 100 + itemIndex Then Print "FAIL descriptor preserve first 0": System 1
    If rootItems(0).leaves(itemIndex).values(1) <> 200 + itemIndex Then Print "FAIL descriptor preserve first 1": System 1
    If rootItems(1).leaves(itemIndex).values(0) <> 300 + itemIndex Then Print "FAIL descriptor preserve second 0": System 1
    If rootItems(1).leaves(itemIndex).values(1) <> 400 + itemIndex Then Print "FAIL descriptor preserve second 1": System 1
    If rootItems(2).leaves(itemIndex).values(0) <> 0 Then Print "FAIL descriptor preserve fresh 0": System 1
    If rootItems(2).leaves(itemIndex).values(1) <> 0 Then Print "FAIL descriptor preserve fresh 1": System 1
Next itemIndex
If rootItems(0).marker <> 11 Or rootItems(1).marker <> 22 Then Print "FAIL marker preserve": System 1

' Make retained elements distinct, then shift bounds so _RETAIN must use coordinates.
For itemIndex = 0 To 2
    rootItems(2).leaves(itemIndex).values(0) = 500 + itemIndex
    rootItems(2).leaves(itemIndex).values(1) = 600 + itemIndex
Next itemIndex
rootItems(2).marker = 33

ReDim _Retain rootItems(1 To 4) As RawDescRoot
For itemIndex = 0 To 2
    If rootItems(1).leaves(itemIndex).values(0) <> 300 + itemIndex Then Print "FAIL descriptor retain first 0": System 1
    If rootItems(1).leaves(itemIndex).values(1) <> 400 + itemIndex Then Print "FAIL descriptor retain first 1": System 1
    If rootItems(2).leaves(itemIndex).values(0) <> 500 + itemIndex Then Print "FAIL descriptor retain second 0": System 1
    If rootItems(2).leaves(itemIndex).values(1) <> 600 + itemIndex Then Print "FAIL descriptor retain second 1": System 1
    If rootItems(3).leaves(itemIndex).values(0) <> 0 Then Print "FAIL descriptor retain fresh 0": System 1
Next itemIndex
If rootItems(1).marker <> 22 Or rootItems(2).marker <> 33 Then Print "FAIL marker retain": System 1

' Retained descriptor graphs must remain independent.
rootItems(1).leaves(0).values(0) = 777
If rootItems(2).leaves(0).values(0) <> 500 Then Print "FAIL descriptor alias": System 1

' Plain REDIM invokes old descriptor cleanup but must create a fresh valid graph afterward.
ReDim rootItems(0 To 1) As RawDescRoot
For itemIndex = 0 To 2
    If rootItems(0).leaves(itemIndex).values(0) <> 0 Then Print "FAIL descriptor plain redim 0": System 1
    If rootItems(1).leaves(itemIndex).values(1) <> 0 Then Print "FAIL descriptor plain redim 1": System 1
Next itemIndex
If rootItems(0).marker <> 0 Or rootItems(1).marker <> 0 Then Print "FAIL marker plain redim": System 1

rootItems(0).leaves(2).values(1) = 909
If rootItems(0).leaves(2).values(1) <> 909 Then Print "FAIL descriptor reuse after redim": System 1

Print "PASS t_qb64pe_refactor_raw_redim_descriptor"
System
