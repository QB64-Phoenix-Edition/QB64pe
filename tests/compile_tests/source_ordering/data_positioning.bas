'Tests DATA may appear in any position with respect to main and sub procedures
$Console:Only

label1: Data 1,2,3,4

For i = 1 To 4
    Read x%
    Print x%
Next i

label2: Data 5,6,7,8

Sub s
    For i = 1 To 8
        Read x%
        Print x%
    Next i
End Sub

s
Restore label1
Read x%
Print x%
Restore label2
Read x%
Print x%
Restore label3
Read x%
Print x%
System

label3: Data 9,10,11,12
