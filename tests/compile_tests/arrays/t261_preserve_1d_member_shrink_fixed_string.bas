$Console:Only
OPTION _EXPLICIT

TYPE T261
    names(0 TO 4) _Dynamic AS STRING * 6
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T261

work(0).names(0) = "AA0000"
work(0).names(1) = "BB1111"
work(0).names(2) = "CC2222"
work(0).names(3) = "DD3333"
work(0).names(4) = "EE4444"

REDIM _PRESERVE work(0).names(0 TO 2)

ok = (LBOUND(work(0).names) = 0 AND UBOUND(work(0).names) = 2)
ok = ok AND (work(0).names(0) = "AA0000" AND work(0).names(1) = "BB1111" AND work(0).names(2) = "CC2222")

IF ok THEN PRINT "PASS t261_preserve_1d_member_shrink_fixed_string" ELSE PRINT "FAIL t261_preserve_1d_member_shrink_fixed_string"
SYSTEM
