'Tests Def* statements apply to SUBs in source file ordering
$Console:Only

Sub a
    x = 34
    Print x!;
End Sub

a
DefLng A-Z

Sub b
    x = 56
    Print x&;
End Sub

b
System
