$CONSOLE:ONLY
Option _Explicit

Type T
    Values(3) As Long
End Type

ReDim a(3) As Long
ReDim x(0) As T

If LBound(a) <> 0 Or UBound(a) <> 3 Then
    Print "FAIL 01_default_base0_normal_and_classic_udt.bas: normal array bounds wrong"
    System
End If

If LBound(x(0).Values) <> 0 Or UBound(x(0).Values) <> 3 Then
    Print "FAIL 01_default_base0_normal_and_classic_udt.bas: member array bounds wrong"
    System
End If

Print "PASS 01_default_base0_normal_and_classic_udt.bas"
System
