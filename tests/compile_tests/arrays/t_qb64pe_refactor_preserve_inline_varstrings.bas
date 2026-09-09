$Console:Only

Type PreserveStringBlock
    texts(0 To 8191) As String
    marker As Long
End Type

ReDim blocks(0 To 0) As PreserveStringBlock

blocks(0).texts(0) = "first"
blocks(0).texts(4095) = "middle"
blocks(0).texts(8191) = "last"
blocks(0).marker = 12345

ReDim _Preserve blocks(0 To 1) As PreserveStringBlock

If blocks(0).texts(0) <> "first" Then Print "FAIL preserve first": System 1
If blocks(0).texts(4095) <> "middle" Then Print "FAIL preserve middle": System 1
If blocks(0).texts(8191) <> "last" Then Print "FAIL preserve last": System 1
If blocks(0).marker <> 12345 Then Print "FAIL preserve fixed scalar": System 1
If blocks(1).texts(0) <> "" Then Print "FAIL new first not empty": System 1
If blocks(1).texts(4095) <> "" Then Print "FAIL new middle not empty": System 1
If blocks(1).texts(8191) <> "" Then Print "FAIL new last not empty": System 1
If blocks(1).marker <> 0 Then Print "FAIL new scalar not zero": System 1

Print "PASS t_qb64pe_refactor_preserve_inline_varstrings"
System
