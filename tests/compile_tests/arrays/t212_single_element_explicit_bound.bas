$Console:Only
OPTION _EXPLICIT

TYPE T212
    nums(4 TO 4) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T212

work(0).nums(4) = 444
ok = (LBOUND(work(0).nums) = 4 AND UBOUND(work(0).nums) = 4 AND work(0).nums(4) = 444)
IF ok THEN PRINT "PASS t212_single_element_explicit_bound" ELSE PRINT "FAIL t212_single_element_explicit_bound"
SYSTEM
