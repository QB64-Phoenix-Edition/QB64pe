$CONSOLE:ONLY
Dim flag As _Bit
flag = -1
If BitToLong&(flag) <> 1 Then Print "FAIL FUNCTION true": End
flag = 0
If BitToLong&(flag) <> 0 Then Print "FAIL FUNCTION false": End
Print "PASS"
System


Function BitToLong& (value As _Bit)
    If value <> 0 Then
        BitToLong& = 1
    Else
        BitToLong& = 0
    End If
End Function
