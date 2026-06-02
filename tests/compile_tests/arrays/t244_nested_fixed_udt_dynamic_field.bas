$Console:Only
OPTION _EXPLICIT

TYPE P244
    xVal AS LONG
    yVal AS LONG
END TYPE

TYPE T244
    pts(1 TO 2) _DynamicField AS P244
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T244

work(0).pts(1).xVal = 10
work(0).pts(1).yVal = 20
work(0).pts(2).xVal = 30
work(0).pts(2).yVal = 40
ok = (work(0).pts(1).xVal + work(0).pts(1).yVal = 30 AND work(0).pts(2).xVal + work(0).pts(2).yVal = 70)
IF ok THEN PRINT "PASS t244_nested_fixed_udt_dynamic_field" ELSE PRINT "FAIL t244_nested_fixed_udt_dynamic_field"
SYSTEM
