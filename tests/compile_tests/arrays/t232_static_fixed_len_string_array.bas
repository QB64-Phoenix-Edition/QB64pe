$Console:Only
OPTION _EXPLICIT

TYPE T232
    codes(0 TO 1) _Static AS STRING * 4
    nums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T232

work(0).codes(0) = "ABCD"
work(0).codes(1) = "EF"
work(0).nums(0) = 12
work(0).nums(1) = 34
ok = (work(0).codes(0) = "ABCD" AND RTRIM$(work(0).codes(1)) = "EF" AND work(0).nums(0) + work(0).nums(1) = 46)
IF ok THEN PRINT "PASS t232_static_fixed_len_string_array" ELSE PRINT "FAIL t232_static_fixed_len_string_array"
SYSTEM
