Option _Explicit
Option Base 1

Type T
    As Long a(3), b(1 To 4)
End Type

ReDim x(0) As T

If LBound(x(0).a) <> 1 Or UBound(x(0).a) <> 3 Then
    Print "FAIL 04_option_base_explicit_bounds.bas: bounds of a are wrong"
    System
End If

If LBound(x(0).b) <> 1 Or UBound(x(0).b) <> 4 Then
    Print "FAIL 04_option_base_explicit_bounds.bas: bounds of b are wrong"
    System
End If

Print "PASS 04_option_base_explicit_bounds.bas"
System
