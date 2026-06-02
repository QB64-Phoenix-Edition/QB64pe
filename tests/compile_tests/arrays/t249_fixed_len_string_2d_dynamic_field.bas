$Console:Only
OPTION _EXPLICIT

TYPE T249
    codes(1 TO 2, 1 TO 2) _DynamicField AS STRING * 3
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T249

work(0).codes(1, 1) = "A"
work(0).codes(1, 2) = "BC"
work(0).codes(2, 1) = "DEF"
work(0).codes(2, 2) = "G"
ok = (RTRIM$(work(0).codes(1, 1)) = "A" AND RTRIM$(work(0).codes(1, 2)) = "BC" AND work(0).codes(2, 1) = "DEF" AND RTRIM$(work(0).codes(2, 2)) = "G")
IF ok THEN PRINT "PASS t249_fixed_len_string_2d_dynamic_field" ELSE PRINT "FAIL t249_fixed_len_string_2d_dynamic_field"
SYSTEM
