$Console:Only
Option _Explicit

Dim a(0 To 50) As String
Dim c(0 To 50) As String
Dim e(0 To 200) As String
Dim h(0 To 40) As String
Dim j(0 To 100) As String

Dim i As Long

For i = 0 To 50
    a(i) = "text:" + LTrim$(Str$(i))
Next

'--------------------------------------------------------------
' Form 1: whole source -> whole destination
'   _ArrayCopy source() TO destination()
'
' Concrete source indices are preserved.
'--------------------------------------------------------------
_ArrayCopy a() To c()

For i = 0 To 50
    If c(i) <> a(i) Then
        Print "FAIL"
        System 1
    End If
Next

'--------------------------------------------------------------
' Form 2: source range -> destination start
'   _ArrayCopy source(lo TO hi) TO destination(start)
'
' The selected source range is copied contiguously beginning
' at the explicit destination index.
'
'   a(20) -> e(50)
'   a(21) -> e(51)
'   ...
'   a(40) -> e(70)
'--------------------------------------------------------------
For i = 0 To 200
    e(i) = "KEEP"
Next

_ArrayCopy a(20 To 40) To e(50)

For i = 20 To 40
    If e(50 + i - 20) <> a(i) Then
        Print "FAIL"
        System 1
    End If
Next

If e(49) <> "KEEP" Or e(71) <> "KEEP" Then
    Print "FAIL"
    System 1
End If

'--------------------------------------------------------------
' Form 3: source range -> whole destination
'   _ArrayCopy source(lo TO hi) TO destination()
'
' The concrete source indices are preserved.
'
'   a(20) -> h(20)
'   ...
'   a(30) -> h(30)
'--------------------------------------------------------------
For i = 0 To 40
    h(i) = "KEEP"
Next

_ArrayCopy a(20 To 30) To h()

For i = 20 To 30
    If h(i) <> a(i) Then
        Print "FAIL"
        System 1
    End If
Next

If h(19) <> "KEEP" Or h(31) <> "KEEP" Then
    Print "FAIL"
    System 1
End If

'--------------------------------------------------------------
' Form 4: whole source -> destination start
'   _ArrayCopy source() TO destination(start)
'
' The whole source array is copied contiguously beginning
' at the explicit destination index.
'
'   a(0)  -> j(30)
'   a(1)  -> j(31)
'   ...
'   a(50) -> j(80)
'--------------------------------------------------------------
For i = 0 To 100
    j(i) = "KEEP"
Next

_ArrayCopy a() To j(30)

For i = 0 To 50
    If j(30 + i) <> a(i) Then
        Print "FAIL"
        System 1
    End If
Next

If j(29) <> "KEEP" Or j(81) <> "KEEP" Then
    Print "FAIL"
    System 1
End If

Print "PASS"
System 0

