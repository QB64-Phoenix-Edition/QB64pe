$Console:Only
Option _Explicit

Dim Shared PassCount As Long
Dim Shared FailCount As Long

Type MultiArrays
    Prefix As Long
    A(1 To 3) As Integer
    Flag As _Unsigned _Byte
    B(- 1 To 1, 2 To 3) As Long
    Tail As Long
    C(0 To 1, 0 To 1, 0 To 1) As _Byte
    S(2) As String * 5
End Type

Dim x(0) As MultiArrays
Dim i As Long
Dim j As Long
Dim k As Long

x(0).Prefix = 1000
x(0).Flag = 200
x(0).Tail = 3000

For i = 1 To 3
    x(0).A(i) = i * 10
Next i

For i = -1 To 1
    For j = 2 To 3
        x(0).B(i, j) = i * 100 + j
    Next j
Next i

For i = 0 To 1
    For j = 0 To 1
        For k = 0 To 1
            x(0).C(i, j, k) = i * 4 + j * 2 + k
        Next k
    Next j
Next i

CheckTrue "A bounds d1", LBound(x(0).A) = 1 And UBound(x(0).A) = 3
CheckTrue "B bounds d1", LBound(x(0).B) = -1 And UBound(x(0).B) = 1
CheckTrue "B bounds d2", LBound(x(0).B, 2) = 2 And UBound(x(0).B, 2) = 3
CheckTrue "C bounds d3", LBound(x(0).C, 3) = 0 And UBound(x(0).C, 3) = 1
CheckTrue "A values", x(0).A(1) = 10 And x(0).A(3) = 30
CheckTrue "B values", x(0).B(-1, 2) = -98 And x(0).B(1, 3) = 103
CheckTrue "C values", x(0).C(0, 1, 1) = 3 And x(0).C(1, 1, 1) = 7
CheckTrue "layout order 1", _Offset(x(0).A) > _Offset(x(0).Prefix)
CheckTrue "layout order 2", _Offset(x(0).Flag) > _Offset(x(0).A)
CheckTrue "layout order 3", _Offset(x(0).B) > _Offset(x(0).Flag)
CheckTrue "layout order 4", _Offset(x(0).Tail) > _Offset(x(0).B)
CheckTrue "layout order 5", _Offset(x(0).C) > _Offset(x(0).Tail)

Erase x(0).B
CheckTrue "Erase member keeps prefix", x(0).Prefix = 1000
CheckTrue "Erase member keeps A", x(0).A(2) = 20
CheckTrue "Erase member zeroes B", x(0).B(-1, 2) = 0 And x(0).B(1, 3) = 0
CheckTrue "Erase member keeps tail", x(0).Tail = 3000
CheckTrue "Erase member keeps C", x(0).C(1, 1, 1) = 7

x(0).S(0) = "hello"
Erase x(0).S
CheckTrue "Erase member sets fixed string to 0s", x(0).S(0) = String$(5, 0)

x(0).S(0) = "hello"
Erase x
CheckTrue "Erase array sets nested fixed string to 0s", x(0).S(0) = String$(5, 0)

FinishTest
'sleep
System

Sub CheckTrue (label As String, condition As Long)
    If condition Then
        PassCount = PassCount + 1
    Else
        FailCount = FailCount + 1
        Print "FAIL:"; label
    End If
End Sub

Sub CheckText (label As String, actual As String, expected As String)
    If actual = expected Then
        PassCount = PassCount + 1
    Else
        FailCount = FailCount + 1
        Print "FAIL:"; label; " expected=["; expected; "] actual=["; actual; "]"
    End If
End Sub

Sub CheckNear (label As String, actual As Double, expected As Double, tolerance As Double)
    If Abs(actual - expected) <= tolerance Then
        PassCount = PassCount + 1
    Else
        FailCount = FailCount + 1
        Print "FAIL:"; label; " expected="; expected; " actual="; actual
    End If
End Sub

Sub FinishTest
    If FailCount = 0 Then
        Print "RESULT: PASS"
    Else
        Print "RESULT: FAIL"; FailCount; "failure(s)"
    End If
End Sub
