$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T267
    labelText AS STRING * 8
    nums(1 TO 2) _Dynamic AS INTEGER
END TYPE

DIM ok AS _BYTE
DIM fileName AS STRING
DIM fileNum AS INTEGER
REDIM src(1 TO 2) AS T267
REDIM dst(1 TO 2) AS T267

fileName = "t267_putget_fixed_scalar_string_nonzero_bounds.bin"
IF _FILEEXISTS(fileName) THEN KILL fileName

src(1).labelText = "FIRST001"
src(1).nums(1) = 111
src(1).nums(2) = 112

src(2).labelText = "SECOND02"
src(2).nums(1) = 221
src(2).nums(2) = 222

fileNum = FREEFILE
OPEN fileName FOR BINARY AS #fileNum
PUT #fileNum, , src()
CLOSE #fileNum

fileNum = FREEFILE
OPEN fileName FOR BINARY AS #fileNum
GET #fileNum, , dst()
CLOSE #fileNum

ok = (LBOUND(dst) = 1 AND UBOUND(dst) = 2)
ok = ok AND (LBOUND(dst(1).nums) = 1 AND UBOUND(dst(1).nums) = 2)
ok = ok AND (dst(1).labelText = "FIRST001" AND dst(1).nums(1) = 111 AND dst(1).nums(2) = 112)
ok = ok AND (dst(2).labelText = "SECOND02" AND dst(2).nums(1) = 221 AND dst(2).nums(2) = 222)

IF _FILEEXISTS(fileName) THEN KILL fileName
IF ok THEN PRINT "PASS t267_putget_fixed_scalar_string_nonzero_bounds" ELSE PRINT "FAIL t267_putget_fixed_scalar_string_nonzero_bounds"
SYSTEM
