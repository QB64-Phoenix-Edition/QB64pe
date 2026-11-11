$CONSOLE:ONLY
Declare Sub ZeroIt (v As _Bit)
Declare Function IsBitSet& (v As _Bit)
Dim flag As _Bit
flag = -1
If IsBitSet&(flag) <> 1 Then Print "FAIL DECLARE FUNCTION": End
ZeroIt flag
If IsBitSet&(flag) <> 0 Then Print "FAIL DECLARE SUB": End
Print "PASS"
System


Sub ZeroIt (v As _Bit)
    v = 0
End Sub

Function IsBitSet& (v As _Bit)
    If v Then IsBitSet& = 1 Else IsBitSet& = 0
End Function
