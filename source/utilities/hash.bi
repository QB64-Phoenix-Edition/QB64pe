
'hash table data
TYPE HashListItem
    Flags AS LONG
    Reference AS LONG
    NextItem AS LONG
    PrevItem AS LONG
    LastItem AS LONG 'note: this value is only valid on the first item in the list
    'note: name is stored in a separate array of strings
END TYPE
DIM SHARED HashFind_NextListItem AS LONG
DIM SHARED HashFind_Reverse AS LONG
DIM SHARED HashFind_SearchFlags AS LONG
DIM SHARED HashFind_Name AS STRING
DIM SHARED HashRemove_LastFound AS LONG
DIM SHARED HashListSize AS LONG
DIM SHARED HashListNext AS LONG
DIM SHARED HashListFreeSize AS LONG
DIM SHARED HashListFreeLast AS LONG
'hash lookup tables
DIM SHARED hash1char(255) AS INTEGER
DIM SHARED hash2char(65535) AS INTEGER
FOR x = 1 TO 26
    hash1char(64 + x) = x
    hash1char(96 + x) = x
NEXT
hash1char(95) = 27 '_
hash1char(48) = 28 '0
hash1char(49) = 29 '1
hash1char(50) = 30 '2
hash1char(51) = 31 '3
hash1char(52) = 23 '4 'note: x, y, z and beginning alphabet letters avoided because of common usage (eg. a2, y3)
hash1char(53) = 22 '5
hash1char(54) = 20 '6
hash1char(55) = 19 '7
hash1char(56) = 18 '8
hash1char(57) = 17 '9
FOR c1 = 0 TO 255
    FOR c2 = 0 TO 255
        hash2char(c1 + c2 * 256) = hash1char(c1) + hash1char(c2) * 32
    NEXT
NEXT
'init
HashListSize = 65536
HashListNext = 1
HashListFreeSize = 1024
HashListFreeLast = 0
REDIM SHARED HashList(1 TO HashListSize) AS HashListItem
REDIM SHARED HashListName(1 TO HashListSize) AS STRING * 256
REDIM SHARED HashListFree(1 TO HashListFreeSize) AS LONG
REDIM SHARED HashTable(16777215) AS LONG '64MB lookup table with indexes to the hashlist

CONST HASHFLAG_LABEL = 2
CONST HASHFLAG_TYPE = 4
CONST HASHFLAG_RESERVED = 8
CONST HASHFLAG_OPERATOR = 16
CONST HASHFLAG_CUSTOMSYNTAX = 32
CONST HASHFLAG_SUB = 64
CONST HASHFLAG_FUNCTION = 128
CONST HASHFLAG_UDT = 256
CONST HASHFLAG_UDTELEMENT = 512
CONST HASHFLAG_CONSTANT = 1024
CONST HASHFLAG_VARIABLE = 2048
CONST HASHFLAG_ARRAY = 4096
CONST HASHFLAG_XELEMENTNAME = 8192
CONST HASHFLAG_XTYPENAME = 16384

'CONST support
DIM SHARED constmax AS LONG
constmax = 100
DIM SHARED constlast AS LONG
constlast = -1
REDIM SHARED constname(constmax) AS STRING
REDIM SHARED constcname(constmax) AS STRING
REDIM SHARED constnamesymbol(constmax) AS STRING 'optional name symbol
' `1 and `no-number must be handled correctly
'DIM SHARED constlastshared AS LONG 'so any defined inside a sub/function after this index can be "forgotten" when sub/function exits
'constlastshared = -1
REDIM SHARED consttype(constmax) AS LONG 'variable type number
'consttype determines storage
REDIM SHARED constinteger(constmax) AS _INTEGER64
REDIM SHARED constuinteger(constmax) AS _UNSIGNED _INTEGER64
REDIM SHARED constfloat(constmax) AS _FLOAT
REDIM SHARED conststring(constmax) AS STRING
REDIM SHARED constsubfunc(constmax) AS LONG
REDIM SHARED constdefined(constmax) AS LONG
