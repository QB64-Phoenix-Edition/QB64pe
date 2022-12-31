$Console:Only
Option _Explicit
Option _ExplicitArray

Dim h As Long: h = _SndNew(1024, 2, 32)
Print "Handle ="; h

Dim m As _MEM: m = _MemSound(h, 0)
Print "Size ="; m.SIZE
Print "Type ="; m.TYPE
Print "Element Size ="; m.ELEMENTSIZE
Print "Sound ="; m.SOUND

System
