$CONSOLE:ONLY

' A block of _MEMNEW(0) owns no memory, so _MEMFREE of it must free nothing.
' The 0-byte block takes the lock of the block freed before it, so a lock
' that kept its old type would free that block's memory a second time.
DIM m AS _MEM, n1 AS _MEM, n2 AS _MEM

m = _MEMNEW(16)
_MEMFREE m

n1 = _MEMNEW(0)
PRINT "n1 size:"; n1.SIZE; "exists:"; _MEMEXISTS(n1)
_MEMFREE n1

n2 = _MEMNEW(0)
_MEMFREE n2

PRINT "after free:"; _MEMEXISTS(n1); _MEMEXISTS(n2)

SYSTEM
