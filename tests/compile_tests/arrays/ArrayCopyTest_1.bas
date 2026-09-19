$Console:Only
$Unstable:TypeFields

Type t
    a As Long
    _Dynamic b(1 to 40) As Single
    c As _Byte
End Type

Dim Pass As _Byte
Dim tt(1) As t
ReDim b(1 To 20)
For e = 1 To 20
    b(e) = e * 3
Next

_ArrayCopy b() To tt(1).b(21)

For f = 21 To 40
    If tt(1).b(f) = (f - 20) * 3 Then Pass = 1 Else Pass = 0: Exit For
Next

If Pass Then Print "PASS" Else Print "FAIL"
System

