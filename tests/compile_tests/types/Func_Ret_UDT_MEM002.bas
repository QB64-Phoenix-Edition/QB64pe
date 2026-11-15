$Console:Only

Dim m As _MEM

m = _MemNew(16)

If m.SIZE <> 16 Then Print "FAIL Func_Ret_UDT_MEM002 direct _MEMNEW size": System 1

_MemFree m

Print "PASS Func_Ret_UDT_MEM002"
System
