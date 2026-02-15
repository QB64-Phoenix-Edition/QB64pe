$Console:Only
'Tests calculation of array and array element sizes

upper = 7

'Single dimension array
Dim a(3 To 7) As Integer
Print Len(a()); Len(a(3))
ReDim ad(3 to upper) As Integer
Print Len(ad()); Len(ad(upper))


'Multi-dimensional array
Dim b(-2 To 3, 0 To 7) As Long
Print Len(b()); Len(b(0, 1))
ReDim bd(-2 To 3, 0 To upper) As Long
Print Len(bd()); Len(bd(0, 1))

'Array of UDT
Type record
    a As Long
    b as Long
End Type
Dim c(3 To 7) As record
Print Len(c()); Len(c(4))
ReDim cd(3 To upper) As record
Print Len(cd()); Len(cd(4))

'Array of fixed strings
Dim d(3 To 7) As String * 10
Print Len(d()); Len(d(4))
ReDim dd(3 To upper) As String * 10
Print Len(dd()); Len(dd(4))

'Array of variable strings
'(only allowed for specific element)
Dim e(3 To 7) As String
e(4) = "hello"
Print Len(e(4))
ReDim ed(3 To upper) As String
ed(4) = "hello"
Print Len(ed(4))

'Specific failing case: test Len function allows maths operators
Dim f(3 To 7) As Long
Print Len(f()) / 4; Len(f()) \ 4; Len(f()) Mod 4
Print Len(f()) + 4; Len(f()) - 4; Len(f()) * 4


System