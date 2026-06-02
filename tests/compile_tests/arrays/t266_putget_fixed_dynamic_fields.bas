$Console:Only
OPTION _EXPLICIT

TYPE T266
    codes(0 TO 1) _DynamicField AS STRING * 4
    nums(0 TO 2) _DynamicField AS LONG
END TYPE

DIM ok AS _BYTE
DIM fileName AS STRING
DIM fileNum AS INTEGER
REDIM src(0 TO 1) AS T266
REDIM dst(0 TO 1) AS T266

fileName = "t266_putget_fixed_dynamic_fields.bin"
IF _FILEEXISTS(fileName) THEN KILL fileName

src(0).codes(0) = "A000"
src(0).codes(1) = "A111"
src(0).nums(0) = 10
src(0).nums(1) = 11
src(0).nums(2) = 12

src(1).codes(0) = "B000"
src(1).codes(1) = "B111"
src(1).nums(0) = 20
src(1).nums(1) = 21
src(1).nums(2) = 22

fileNum = FREEFILE
OPEN fileName FOR BINARY AS #fileNum
PUT #fileNum, , src()
CLOSE #fileNum

fileNum = FREEFILE
OPEN fileName FOR BINARY AS #fileNum
GET #fileNum, , dst()
CLOSE #fileNum

ok = (dst(0).codes(0) = "A000" AND dst(0).codes(1) = "A111")
ok = ok AND (dst(0).nums(0) = 10 AND dst(0).nums(1) = 11 AND dst(0).nums(2) = 12)
ok = ok AND (dst(1).codes(0) = "B000" AND dst(1).codes(1) = "B111")
ok = ok AND (dst(1).nums(0) = 20 AND dst(1).nums(1) = 21 AND dst(1).nums(2) = 22)

IF _FILEEXISTS(fileName) THEN KILL fileName
IF ok THEN PRINT "PASS t266_putget_fixed_dynamic_fields" ELSE PRINT "FAIL t266_putget_fixed_dynamic_fields"
SYSTEM
