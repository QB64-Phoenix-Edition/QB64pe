$CONSOLE:ONLY
Dim flag As _Bit
flag = -1
ClearBit flag
If flag <> 0 Then Print "FAIL SUB _BIT": End
Print "PASS"
System

Sub ClearBit (value As _Bit)
    value = 0
End Sub
