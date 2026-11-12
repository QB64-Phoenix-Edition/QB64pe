$CONSOLE:ONLY

' This program does some general testing of the split C++ generation by having
' several separate source files that declare some global information and some
' SUBs/FUNCTIONs. It should compile and produce the same result with and
' without splitting being enabled.

'$include:'a.bi'

gCount = 10
gRec.n = 7
gRec.s = "shared"

ASub1
AssertBool "a reached b", gCount = 11

AssertString "b direct", BFunc1$(3), "B-ONE-3-12"

CSub1
AssertBool "c reached a and b", gCount = 13

AssertBool "c dynamic array", CFunc2%(5) = 5

AssertBool "static kept counting", gBCalls = 2

SYSTEM

'$include:'../utilities/assert.bm'
'$include:'a.bm'
'$include:'b.bm'
'$include:'c.bm'
