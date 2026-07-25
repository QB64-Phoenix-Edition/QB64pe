$Console:Only
$Unstable:TypeFields

Option _Explicit

Type T272
    tags(0 TO 1) _Static As String * 5
    values(0 TO 1) _Dynamic As Long
End Type

Dim ok As _Byte
ReDim src(0 To 1) As T272
ReDim dst(0 To 1) As T272

src(0).tags(0) = "S0A"
src(0).tags(1) = "S0B"
ReDim src(0).values(0 TO 2)
src(0).values(0) = 100
src(0).values(1) = 101
src(0).values(2) = 102

src(1).tags(0) = "S1A"
src(1).tags(1) = "S1B"
ReDim src(1).values(5 TO 6)
src(1).values(5) = 205
src(1).values(6) = 206

dst() = src()

src(0).tags(0) = "BAD0"
src(0).values(0) = -1
src(1).tags(1) = "BAD1"
src(1).values(6) = -2

ok = (dst(0).tags(0) = "S0A  " And dst(0).tags(1) = "S0B  ")
ok = ok And (LBound(dst(0).values) = 0 And UBound(dst(0).values) = 2)
ok = ok And (dst(0).values(0) = 100 And dst(0).values(1) = 101 And dst(0).values(2) = 102)
ok = ok And (dst(1).tags(0) = "S1A  " And dst(1).tags(1) = "S1B  ")
ok = ok And (LBound(dst(1).values) = 5 And UBound(dst(1).values) = 6)
ok = ok And (dst(1).values(5) = 205 And dst(1).values(6) = 206)

If ok Then Print "PASS t272_dynamic_static_array_assign_deepcopy" Else Print "FAIL t272_dynamic_static_array_assign_deepcopy"
System
