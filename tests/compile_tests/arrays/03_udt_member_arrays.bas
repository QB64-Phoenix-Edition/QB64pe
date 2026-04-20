$CONSOLE:ONLY
Option _Explicit

Type InnerType
    As Integer q(2), r
End Type

Type OuterType
    As InnerType n(1), m
End Type

ReDim x(0) As OuterType

If LBound(x(0).n) <> 0 Or UBound(x(0).n) <> 1 Then
    Print "FAIL 03_udt_member_arrays.bas: bounds of n are wrong"
    System
End If

If LBound(x(0).n(1).q) <> 0 Or UBound(x(0).n(1).q) <> 2 Then
    Print "FAIL 03_udt_member_arrays.bas: bounds of q are wrong"
    System
End If

x(0).n(1).q(2) = 77
x(0).m.r = 88

If x(0).n(1).q(2) <> 77 Or x(0).m.r <> 88 Then
    Print "FAIL 03_udt_member_arrays.bas: nested access failed"
    System
End If

Print "PASS 03_udt_member_arrays.bas"
System
