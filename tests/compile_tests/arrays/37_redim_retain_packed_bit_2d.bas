$Console:Only
Option _Explicit

Dim row As Long
Dim col As Long
Dim expected As Long
ReDim packedValues(-1 To 1, 5 To 8) As _Unsigned _Bit * 5

For row = -1 To 1
    For col = 5 To 8
        packedValues(row, col) = (row + 1) * 8 + (col - 5) + 1
    Next col
Next row

' _RETAIN preserves coordinates, not flattened byte positions. Packed _BIT
' arrays therefore have to copy each logical element by bit index.
ReDim _Retain packedValues(0 To 2, 6 To 9) As _Unsigned _Bit * 5

If LBound(packedValues, 1) <> 0 Then Print "FAIL 37_redim_retain_packed_bit_2d.bas": System
If UBound(packedValues, 1) <> 2 Then Print "FAIL 37_redim_retain_packed_bit_2d.bas": System
If LBound(packedValues, 2) <> 6 Then Print "FAIL 37_redim_retain_packed_bit_2d.bas": System
If UBound(packedValues, 2) <> 9 Then Print "FAIL 37_redim_retain_packed_bit_2d.bas": System

For row = 0 To 1
    For col = 6 To 8
        expected = (row + 1) * 8 + (col - 5) + 1
        If packedValues(row, col) <> expected Then Print "FAIL 37_redim_retain_packed_bit_2d.bas": System
    Next col
Next row

' Coordinates introduced by the new bounds must be zero-initialized.
For row = 0 To 1
    If packedValues(row, 9) <> 0 Then Print "FAIL 37_redim_retain_packed_bit_2d.bas": System
Next row
For col = 6 To 9
    If packedValues(2, col) <> 0 Then Print "FAIL 37_redim_retain_packed_bit_2d.bas": System
Next col

Print "PASS 37_redim_retain_packed_bit_2d.bas"
System
