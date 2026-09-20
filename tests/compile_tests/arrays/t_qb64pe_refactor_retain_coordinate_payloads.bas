$Console:Only
Option _Explicit

Dim row As Long
Dim col As Long
Dim expectedText As String
Dim expectedValue As Long

' Variable STRING payload: _RETAIN must preserve the coordinate intersection
' with ownership-aware qbs_set rather than flattened-position copying.
ReDim textData(-1 To 1, 5 To 7) As String
For row = -1 To 1
    For col = 5 To 7
        textData(row, col) = "T" + LTrim$(Str$(row)) + ":" + LTrim$(Str$(col))
    Next col
Next row

ReDim _Retain textData(0 To 2, 6 To 8) As String

For row = 0 To 1
    For col = 6 To 7
        expectedText = "T" + LTrim$(Str$(row)) + ":" + LTrim$(Str$(col))
        If textData(row, col) <> expectedText Then Print "FAIL string intersection": System 1
    Next col
Next row
For row = 0 To 2
    If textData(row, 8) <> "" Then Print "FAIL string new column": System 1
Next row
For col = 6 To 8
    If textData(2, col) <> "" Then Print "FAIL string new row": System 1
Next col

' Fixed numeric payload uses the same coordinate walker but keeps raw element memcpy.
ReDim numberData(-1 To 1, 5 To 7) As Long
For row = -1 To 1
    For col = 5 To 7
        numberData(row, col) = (row + 2) * 100 + col
    Next col
Next row

ReDim _Retain numberData(0 To 2, 6 To 8) As Long

For row = 0 To 1
    For col = 6 To 7
        expectedValue = (row + 2) * 100 + col
        If numberData(row, col) <> expectedValue Then Print "FAIL numeric intersection": System 1
    Next col
Next row
For row = 0 To 2
    If numberData(row, 8) <> 0 Then Print "FAIL numeric new column": System 1
Next row
For col = 6 To 8
    If numberData(2, col) <> 0 Then Print "FAIL numeric new row": System 1
Next col

' Packed _BIT payload must use logical getubits/setbits indexes inside the same walker.
ReDim packedData(-1 To 1, 5 To 7) As _Unsigned _Bit * 5
For row = -1 To 1
    For col = 5 To 7
        packedData(row, col) = (row + 1) * 8 + (col - 5) + 1
    Next col
Next row

ReDim _Retain packedData(0 To 2, 6 To 8) As _Unsigned _Bit * 5

For row = 0 To 1
    For col = 6 To 7
        expectedValue = (row + 1) * 8 + (col - 5) + 1
        If packedData(row, col) <> expectedValue Then Print "FAIL bit intersection": System 1
    Next col
Next row
For row = 0 To 2
    If packedData(row, 8) <> 0 Then Print "FAIL bit new column": System 1
Next row
For col = 6 To 8
    If packedData(2, col) <> 0 Then Print "FAIL bit new row": System 1
Next col

Print "PASS t_qb64pe_refactor_retain_coordinate_payloads"
System
