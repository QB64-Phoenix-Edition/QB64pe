$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T216
    cube(0 TO 1, 0 TO 1, 0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T216

work(0).cube(0, 0, 0) = 1
work(0).cube(1, 1, 1) = 8
ok = (LBOUND(work(0).cube, 3) = 0 AND UBOUND(work(0).cube, 3) = 1 AND work(0).cube(0, 0, 0) = 1 AND work(0).cube(1, 1, 1) = 8)
IF ok THEN PRINT "PASS t216_three_dim_small_cube" ELSE PRINT "FAIL t216_three_dim_small_cube"
SYSTEM
