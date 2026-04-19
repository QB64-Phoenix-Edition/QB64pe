$CONSOLE:ONLY
Option _Explicit

Type T
    As String * 8 s ( 3 ), t
End Type

ReDim x(0) As T
Dim i As Long

If LBound(x(0).s) <> 0 Or UBound(x(0).s) <> 3 Then
    Print "FAIL 02_fixed_string_and_scalars.bas: bounds are wrong"
    System
End If

x(0).s(0) = "AB"
x(0).s(3) = "XYZ"
x(0).t = "KOCKA"

If RTrim$(x(0).s(0)) <> "AB" Or RTrim$(x(0).s(3)) <> "XYZ" Or RTrim$(x(0).t) <> "KOCKA" Then 'static string array....
    Print "FAIL 02_fixed_string_and_scalars.bas: assignment failed"
    'Print x(0).s(0), x(0).s(3), x(0).t
    'Sleep
    System
End If

Erase x(0).s
For i = 0 To 3
    If x(0).s(i) <> Chr$(0) + Chr$(0) + Chr$(0) + Chr$(0) + Chr$(0) + Chr$(0) + Chr$(0) + Chr$(0) Then
        Print "FAIL 02_fixed_string_and_scalars.bas: erase mismatch at index"; i
        System
    End If
Next i

Print "PASS 02_fixed_string_and_scalars.bas"
System
