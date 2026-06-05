$Console:Only
OPTION _EXPLICIT

TYPE T242
    nums(0 TO 2) _Dynamic AS LONG
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T242

work(0).nums(0) = 5
work(0).nums(2) = 9
SWAP work(0).nums(0), work(0).nums(2)
ok = (work(0).nums(0) = 9 AND work(0).nums(2) = 5)
IF ok THEN PRINT "PASS t242_swap_indexed_member_values" ELSE PRINT "FAIL t242_swap_indexed_member_values"
SYSTEM
