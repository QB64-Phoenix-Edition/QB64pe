$Console:Only
OPTION _EXPLICIT

TYPE T215
    grid(2 TO 3, -1 TO 1) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T215

work(0).grid(2, -1) = 21
work(0).grid(3, 1) = 31
ok = (LBOUND(work(0).grid, 1) = 2 AND UBOUND(work(0).grid, 1) = 3 AND LBOUND(work(0).grid, 2) = -1 AND UBOUND(work(0).grid, 2) = 1 AND work(0).grid(2, -1) = 21 AND work(0).grid(3, 1) = 31)
IF ok THEN PRINT "PASS t215_two_dim_mixed_bounds" ELSE PRINT "FAIL t215_two_dim_mixed_bounds"
SYSTEM
