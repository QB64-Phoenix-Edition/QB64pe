$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T234
    textItems(0 TO 2) AS STRING
END TYPE

DIM ok AS _BYTE
REDIM work(0) AS T234

work(0).textItems(0) = "zero"
work(0).textItems(1) = "one"
work(0).textItems(2) = "two"
ok = (work(0).textItems(0) = "zero" AND work(0).textItems(1) = "one" AND work(0).textItems(2) = "two")
IF ok THEN PRINT "PASS t234_unmarked_inline_varstring_array_redim_parent" ELSE PRINT "FAIL t234_unmarked_inline_varstring_array_redim_parent"
SYSTEM
