'Tests $checking:off applies to SUBs in source file ordering
$Console:Only

$Checking:Off
Function deref& (p As _Offset)
    Dim m As _MEM
    _MemGet m, p, r& 'Operation would error with $checking:on
    deref& = r&
End Function
$Checking:On

x& = 12
Print deref&(_Offset(x&));
System
