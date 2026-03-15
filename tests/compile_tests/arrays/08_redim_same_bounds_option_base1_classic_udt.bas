Option _Explicit
Option Base 1

Type T
    Values( 3) As Long
End Type

ReDim reference(3) As Long
ReDim x(0) As T
Dim i As Long

For i = 1 To 3
    reference(i) = i + 100
    x(0).Values(i) = i + 100 'valid test: program crash with error message, OK
Next i

ReDim reference(3)
ReDim x(0).Values(3)

For i = 1 To 3
    If x(0).Values(i) <> reference(i) Then
        Print "FAIL 08_redim_same_bounds_option_base1_classic_udt.bas: index"; i; " expected "; reference(i); " got "; x(0).Values(i)
        System
    End If
Next i

Print "PASS 08_redim_same_bounds_option_base1_classic_udt.bas"
System
