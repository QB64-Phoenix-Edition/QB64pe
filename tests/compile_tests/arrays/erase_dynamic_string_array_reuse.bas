$CONSOLE:ONLY
OPTION _EXPLICIT

' Exercises the free/realloc/init cycle for a dynamic array of
' variable-length strings, where each element is one qbs* slot.

REDIM a(1 TO 3) AS STRING
a(1) = "red"
a(2) = "green"
a(3) = "blue"
AssertString "before erase", a(2), "green"

REDIM a(1 TO 2) AS STRING
AssertString "element after redim", a(1), ""

a(1) = "again"
a(2) = "once more"
AssertString "assign after redim", a(1), "again"
AssertString "assign after redim 2", a(2), "once more"

ERASE a
REDIM a(1 TO 1) ' We can't check for uninitialized arrays, so a REDIM after the ERASE is necessary
AssertString "erase/redim clears array contents", a(1), ""

REDIM a(0 TO 0) AS STRING
a(0) = "single"
AssertString "single element", a(0), "single"

REDIM b(1 TO 4) AS STRING
b(1) = "one"
b(4) = "four"
REDIM _PRESERVE b(1 TO 6) AS STRING
AssertString "preserve keeps first", b(1), "one"
AssertString "preserve keeps last", b(4), "four"
AssertString "preserve clears grown", b(6), ""

b(6) = "six"
AssertString "grown element usable", b(6), "six"

REDIM _PRESERVE b(1 TO 2) AS STRING
AssertString "shrink keeps first", b(1), "one"

SYSTEM

'$include:'../utilities/assert.bm'
