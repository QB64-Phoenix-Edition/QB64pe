Option _Explicit
Option Base 1

Type T
    Grid(2, 3) As Long
End Type

ReDim x(0) As T

If LBound(x(0).Grid, 1) <> 1 Or UBound(x(0).Grid, 1) <> 2 Then
    Print "FAIL 06_option_base1_classic_udt_two_dimensions.bas: dim1 bounds wrong"
    System
End If

If LBound(x(0).Grid, 2) <> 1 Or UBound(x(0).Grid, 2) <> 3 Then
    Print "FAIL 06_option_base1_classic_udt_two_dimensions.bas: dim2 bounds wrong"
    System
End If

Print "PASS 06_option_base1_classic_udt_two_dimensions.bas"
System
