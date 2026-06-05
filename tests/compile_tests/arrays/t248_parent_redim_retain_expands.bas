$Console:Only
OPTION _EXPLICIT

TYPE T248
    nums(0 TO 1) _Dynamic AS LONG
    noteText AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work(0 TO 0) AS T248

work(0).nums(0) = 12
work(0).noteText = "keep"
REDIM _RETAIN work(0 TO 1) AS T248
work(1).nums(1) = 34
work(1).noteText = "new"
ok = (work(0).nums(0) = 12 AND work(0).noteText = "keep" AND work(1).nums(0) = 0 AND work(1).nums(1) = 34 AND work(1).noteText = "new")
IF ok THEN PRINT "PASS t248_parent_redim_retain_expands" ELSE PRINT "FAIL t248_parent_redim_retain_expands"
SYSTEM
