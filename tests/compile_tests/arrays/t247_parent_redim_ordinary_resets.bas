$Console:Only
OPTION _EXPLICIT

TYPE T247
    nums(0 TO 1) _DynamicField AS LONG
    noteText AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T247

work(0).nums(0) = 77
work(0).noteText = "old"
REDIM work(0) AS T247
ok = (work(0).nums(0) = 0 AND work(0).nums(1) = 0 AND work(0).noteText = "")
IF ok THEN PRINT "PASS t247_parent_redim_ordinary_resets" ELSE PRINT "FAIL t247_parent_redim_ordinary_resets"
SYSTEM
