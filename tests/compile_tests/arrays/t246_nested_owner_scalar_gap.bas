$Console:Only
OPTION _EXPLICIT

TYPE Leaf246
    numsA(0 TO 1) _Dynamic AS LONG
    gapText AS STRING
    numsB(0 TO 1) _Dynamic AS LONG
END TYPE

TYPE T246
    item(0 TO 1) _Dynamic AS Leaf246
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T246

work(0).item(0).numsA(1) = 12
work(0).item(0).gapText = "gap"
work(0).item(0).numsB(0) = 34
work(0).item(1).gapText = "second"
work(0).item(1).numsB(1) = 56
ok = (work(0).item(0).numsA(1) = 12 AND work(0).item(0).gapText = "gap" AND work(0).item(0).numsB(0) = 34 AND work(0).item(1).gapText = "second" AND work(0).item(1).numsB(1) = 56)
IF ok THEN PRINT "PASS t246_nested_owner_scalar_gap" ELSE PRINT "FAIL t246_nested_owner_scalar_gap"
SYSTEM
