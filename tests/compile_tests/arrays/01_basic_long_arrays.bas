Option _Explicit

Type T
    As Long a(12, 15), b(44, 1, 5), c
End Type

ReDim x(0) As T

If LBound(x(0).a, 1) <> 0 Or UBound(x(0).a, 1) <> 12 Or LBound(x(0).a, 2) <> 0 Or UBound(x(0).a, 2) <> 15 Then
    Print "FAIL 01_basic_long_arrays.bas: bounds of a are wrong"
    System
End If

If LBound(x(0).b, 1) <> 0 Or UBound(x(0).b, 1) <> 44 Then
    Print "FAIL 01_basic_long_arrays.bas: bounds of b dim 1 are wrong"
    System
End If

If LBound(x(0).b, 2) <> 0 Or UBound(x(0).b, 2) <> 1 Then
    Print "FAIL 01_basic_long_arrays.bas: bounds of b dim 2 are wrong"
    System
End If

If LBound(x(0).b, 3) <> 0 Or UBound(x(0).b, 3) <> 5 Then
    Print "FAIL 01_basic_long_arrays.bas: bounds of b dim 3 are wrong"
    System
End If

x(0).a(12, 15) = 1234
x(0).b(44, 1, 5) = 5678
x(0).c = 9

If x(0).a(12, 15) <> 1234 Or x(0).b(44, 1, 5) <> 5678 Or x(0).c <> 9 Then
    Print "FAIL 01_basic_long_arrays.bas: data access failed"
    System
End If

Print "PASS 01_basic_long_arrays.bas"
System
