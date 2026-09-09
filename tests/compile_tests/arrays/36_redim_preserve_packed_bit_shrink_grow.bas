$Console:Only
Option _Explicit

Dim idx As Long
ReDim packedBits(0 To 7) As _Unsigned _Bit

' Set bits that will later be removed. A byte-wise _PRESERVE copy used to keep
' those packed bits alive inside the shrunken allocation, so they reappeared
' when the array was grown again.
packedBits(0) = 1
packedBits(1) = 0
packedBits(2) = 1
For idx = 3 To 7
    packedBits(idx) = 1
Next idx

ReDim _Preserve packedBits(0 To 2) As _Unsigned _Bit

If packedBits(0) <> 1 Then Print "FAIL 36_redim_preserve_packed_bit_shrink_grow.bas": System
If packedBits(1) <> 0 Then Print "FAIL 36_redim_preserve_packed_bit_shrink_grow.bas": System
If packedBits(2) <> 1 Then Print "FAIL 36_redim_preserve_packed_bit_shrink_grow.bas": System

ReDim _Preserve packedBits(0 To 7) As _Unsigned _Bit

If packedBits(0) <> 1 Then Print "FAIL 36_redim_preserve_packed_bit_shrink_grow.bas": System
If packedBits(1) <> 0 Then Print "FAIL 36_redim_preserve_packed_bit_shrink_grow.bas": System
If packedBits(2) <> 1 Then Print "FAIL 36_redim_preserve_packed_bit_shrink_grow.bas": System
For idx = 3 To 7
    If packedBits(idx) <> 0 Then Print "FAIL 36_redim_preserve_packed_bit_shrink_grow.bas": System
Next idx

Print "PASS 36_redim_preserve_packed_bit_shrink_grow.bas"
System
