$Console:Only
OPTION _EXPLICIT

TYPE P245
    xVal AS LONG
    yVal AS LONG
END TYPE

TYPE T245
    pts(0 TO 1, 0 TO 1) _DynamicField AS P245
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T245

work(0).pts(0, 0).xVal = 1
work(0).pts(0, 0).yVal = 2
work(0).pts(1, 1).xVal = 3
work(0).pts(1, 1).yVal = 4
ok = (work(0).pts(0, 0).xVal = 1 AND work(0).pts(0, 0).yVal = 2 AND work(0).pts(1, 1).xVal = 3 AND work(0).pts(1, 1).yVal = 4)
IF ok THEN PRINT "PASS t245_nested_fixed_udt_2d_dynamic_field" ELSE PRINT "FAIL t245_nested_fixed_udt_2d_dynamic_field"
SYSTEM
