$CONSOLE:ONLY
Option _Explicit
Option Base 1

Type TClassic
    Values(0 To 3) As Long
End Type

Type TAsList
    As Long Values(0 To 3), Scalar
End Type

ReDim a(0 To 3) As Long
ReDim x1(0) As TClassic
ReDim x2(0) As TAsList

If LBound(a) <> 0 Or UBound(a) <> 3 Then
    Print "FAIL 05_explicit_bounds_ignore_option_base1.bas: normal explicit bounds wrong"
    System
End If

If LBound(x1(0).Values) <> 0 Or UBound(x1(0).Values) <> 3 Then
    Print "FAIL 05_explicit_bounds_ignore_option_base1.bas: classic member explicit bounds wrong"
    System
End If

If LBound(x2(0).Values) <> 0 Or UBound(x2(0).Values) <> 3 Then
    Print "FAIL 05_explicit_bounds_ignore_option_base1.bas: as-list member explicit bounds wrong"
    System
End If

Print "PASS 05_explicit_bounds_ignore_option_base1.bas"
System
