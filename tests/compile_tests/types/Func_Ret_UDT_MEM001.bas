$Console:Only

Dim m As _MEM
Dim x As Long
Dim y As Long

m = MakeMem(16)

x = &H12345678
_MemPut m, m.OFFSET, x
_MemGet m, m.OFFSET, y

If m.SIZE <> 16 Then Print "FAIL Func_Ret_UDT_MEM001 size": System 1
If y <> x Then Print "FAIL Func_Ret_UDT_MEM001 payload": System 1

_MemFree m

Print "PASS Func_Ret_UDT_MEM001"
System

Function MakeMem (bytes As Long) As _MEM
    MakeMem = _MemNew(bytes)
End Function
