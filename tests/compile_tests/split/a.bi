' A file that contributes types, constants and globals but no routines.
' Grouping is by the file a routine is declared in, so this one should not produce
' a unit of its own.

CONST ATag = "A-ONE"

TYPE MyType
    n AS LONG
    s AS STRING * 8
END TYPE

' Reached from every unit, so each of these is one definition plus a declaration
' in every translation unit that sees it
DIM SHARED gCount AS LONG
DIM SHARED gRec AS MyType
DIM SHARED gBCalls AS LONG
