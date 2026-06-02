$Console:Only
OPTION _EXPLICIT

TYPE T233
    nums(0 TO 2) AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T233

work(0).nums(0) = 10
work(0).nums(1) = 20
work(0).nums(2) = 30
ok = (work(0).nums(0) + work(0).nums(1) + work(0).nums(2) = 60)
IF ok THEN PRINT "PASS t233_unmarked_inline_numeric_array_redim_parent" ELSE PRINT "FAIL t233_unmarked_inline_numeric_array_redim_parent"
SYSTEM
