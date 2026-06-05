$Console:Only
OPTION _EXPLICIT

TYPE T231
    codes(1 TO 3) _Dynamic AS STRING * 5
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T231

work(0).codes(1) = "ALPHA"
work(0).codes(2) = "B"
work(0).codes(3) = "GAMMA"
ok = (work(0).codes(1) = "ALPHA" AND RTRIM$(work(0).codes(2)) = "B" AND work(0).codes(3) = "GAMMA")
IF ok THEN PRINT "PASS t231_fixed_len_string_dynamic_field" ELSE PRINT "FAIL t231_fixed_len_string_dynamic_field"
SYSTEM
