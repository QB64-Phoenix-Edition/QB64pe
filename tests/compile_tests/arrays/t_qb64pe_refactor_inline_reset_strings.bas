$Console:Only
$Unstable:TypeFields
Option _Explicit

Type InlineStringHolder
    values(0 To 8191) As String
    markerValue As Long
End Type

Dim holders(0 To 1) As InlineStringHolder

holders(0).markerValue = 111
holders(1).markerValue = 222
holders(0).values(0) = "other parent"
holders(1).values(0) = "first"
holders(1).values(4096) = "middle"
holders(1).values(8191) = "last"

Erase holders(1).values

If holders(1).values(0) <> "" Then Print "FAIL erase first string": System 1
If holders(1).values(4096) <> "" Then Print "FAIL erase middle string": System 1
If holders(1).values(8191) <> "" Then Print "FAIL erase last string": System 1
If holders(1).markerValue <> 222 Then Print "FAIL erase sibling scalar": System 1
If holders(0).values(0) <> "other parent" Then Print "FAIL erase parent isolation": System 1
If holders(0).markerValue <> 111 Then Print "FAIL erase other marker": System 1

holders(1).values(0) = "again first"
holders(1).values(4096) = "again middle"
holders(1).values(8191) = "again last"

ReDim holders(1).values(0 To 8191) As String

If holders(1).values(0) <> "" Then Print "FAIL redim first string": System 1
If holders(1).values(4096) <> "" Then Print "FAIL redim middle string": System 1
If holders(1).values(8191) <> "" Then Print "FAIL redim last string": System 1
If holders(1).markerValue <> 222 Then Print "FAIL redim sibling scalar": System 1
If holders(0).values(0) <> "other parent" Then Print "FAIL redim parent isolation": System 1
If LBound(holders(1).values) <> 0 Or UBound(holders(1).values) <> 8191 Then Print "FAIL redim bounds": System 1

Print "PASS t_qb64pe_refactor_inline_reset_strings"
System
