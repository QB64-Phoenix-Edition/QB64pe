$Console:Only
OPTION _EXPLICIT

TYPE T241
    nums(0 TO 1) _Dynamic AS LONG
    tagText AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 1) AS T241

work(0).nums(0) = 10
work(0).tagText = "zero"
work(1).nums(0) = 20
work(1).tagText = "one"
SWAP work(0), work(1)
ok = (work(0).nums(0) = 20 AND work(0).tagText = "one" AND work(1).nums(0) = 10 AND work(1).tagText = "zero")
IF ok THEN PRINT "PASS t241_swap_parent_elements" ELSE PRINT "FAIL t241_swap_parent_elements"
SYSTEM
