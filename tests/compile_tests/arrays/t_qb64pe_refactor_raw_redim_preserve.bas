$Console:Only
Option _Explicit

Dim i As Long

' Ordinary fixed numeric payload exercises the shared raw REDIM preservation body.
ReDim numbers(-3 To 5) As Long
For i = -3 To 5
    numbers(i) = i * 17 + 200
Next i

ReDim _Preserve numbers(-3 To 9) As Long
For i = -3 To 5
    If numbers(i) <> i * 17 + 200 Then Print "FAIL numeric preserve": System 1
Next i
For i = 6 To 9
    If numbers(i) <> 0 Then Print "FAIL numeric preserve fresh": System 1
Next i

' Plain REDIM must discard the old payload and return zero-filled storage.
ReDim numbers(2 To 6) As Long
For i = 2 To 6
    If numbers(i) <> 0 Then Print "FAIL numeric plain redim": System 1
    numbers(i) = i * 31
Next i

' _RETAIN must preserve by coordinates rather than flattened position.
ReDim _Retain numbers(4 To 8) As Long
For i = 4 To 6
    If numbers(i) <> i * 31 Then Print "FAIL numeric retain": System 1
Next i
For i = 7 To 8
    If numbers(i) <> 0 Then Print "FAIL numeric retain fresh": System 1
Next i

' Packed _BIT uses the same shared body but must copy logical elements, not bytes.
ReDim packed(0 To 15) As _Unsigned _Bit * 5
For i = 0 To 15
    packed(i) = (i * 3 + 1) And 31
Next i

ReDim _Preserve packed(0 To 7) As _Unsigned _Bit * 5
For i = 0 To 7
    If packed(i) <> ((i * 3 + 1) And 31) Then Print "FAIL bit preserve shrink": System 1
Next i

ReDim _Preserve packed(0 To 15) As _Unsigned _Bit * 5
For i = 0 To 7
    If packed(i) <> ((i * 3 + 1) And 31) Then Print "FAIL bit preserve regrow kept": System 1
Next i
For i = 8 To 15
    If packed(i) <> 0 Then Print "FAIL bit preserve zombie": System 1
Next i

Print "PASS t_qb64pe_refactor_raw_redim_preserve"
System
