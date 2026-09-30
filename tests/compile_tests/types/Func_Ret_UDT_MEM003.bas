$Console:Only

Dim m1 As _MEM
Dim m2 As _MEM
Dim a As Long
Dim b As Long
Dim ra As Long
Dim rb As Long

m1 = MakeMem(16)
m2 = MakeMem(16)

a = 111111
b = 222222

_MemPut m1, m1.OFFSET, a
_MemPut m2, m2.OFFSET, b

_MemGet m1, m1.OFFSET, ra
_MemGet m2, m2.OFFSET, rb

If m1.SIZE <> 16 Then Print "FAIL Func_Ret_UDT_MEM003 m1 size": System 1
If m2.SIZE <> 16 Then Print "FAIL Func_Ret_UDT_MEM003 m2 size": System 1
If ra <> a Then Print "FAIL Func_Ret_UDT_MEM003 first payload": System 1
If rb <> b Then Print "FAIL Func_Ret_UDT_MEM003 second payload": System 1
If m1.OFFSET = m2.OFFSET Then Print "FAIL Func_Ret_UDT_MEM003 returned blocks alias": System 1

_MemFree m1
_MemFree m2

Print "PASS Func_Ret_UDT_MEM003"
System

Function MakeMem (bytes As Long) As _MEM
    MakeMem = _MemNew(bytes)
End Function
