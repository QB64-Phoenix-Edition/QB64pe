$CONSOLE:ONLY
OPTION _EXPLICIT

TYPE ClearLeaf
    Texts(0 TO 255) AS STRING
    Number AS LONG
END TYPE

DIM items(0 TO 2) AS ClearLeaf

items(0).Texts(0) = "zero"
items(1).Texts(128) = "middle"
items(2).Texts(255) = "last"
items(1).Number = 12345

' CLEAR must clear live qbs contents and numeric storage without leaving stale owners.
CLEAR

IF items(0).Texts(0) <> "" THEN GOTO failed
IF items(1).Texts(128) <> "" THEN GOTO failed
IF items(2).Texts(255) <> "" THEN GOTO failed
IF items(1).Number <> 0 THEN GOTO failed

' The array must remain usable after CLEAR.
items(2).Texts(255) = "after-clear"
items(2).Number = 99
IF items(2).Texts(255) <> "after-clear" THEN GOTO failed
IF items(2).Number <> 99 THEN GOTO failed

PRINT "PASS 95_type_codegen_clear_inline_owner.bas"
SYSTEM

failed:
PRINT "FAIL 95_type_codegen_clear_inline_owner.bas"
SYSTEM
