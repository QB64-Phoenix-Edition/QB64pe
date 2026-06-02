$Console:Only
OPTION _EXPLICIT

TYPE T219
    headText AS STRING * 4
    nums(0 TO 1) _DynamicField AS LONG
    tailText AS STRING * 4
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T219

work(0).headText = "HEAD"
work(0).nums(0) = 123
work(0).tailText = "TAIL"
ok = (work(0).headText = "HEAD" AND work(0).nums(0) = 123 AND work(0).tailText = "TAIL")
IF ok THEN PRINT "PASS t219_fixed_string_scalar_around_dynfield" ELSE PRINT "FAIL t219_fixed_string_scalar_around_dynfield"
SYSTEM
