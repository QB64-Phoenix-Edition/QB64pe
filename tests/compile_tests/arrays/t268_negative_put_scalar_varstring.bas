$Console:Only
$Unstable:TypeFields

OPTION _EXPLICIT

TYPE T268
    labelText AS STRING
    nums(0 TO 1) _Dynamic AS LONG
END TYPE

DIM fileName AS STRING
DIM fileNum AS INTEGER
REDIM work(0) AS T268

fileName = "t268_negative_put_scalar_varstring.bin"
IF _FILEEXISTS(fileName) THEN KILL fileName

work(0).labelText = "variable string must make PUT unsupported"
work(0).nums(0) = 10
work(0).nums(1) = 20

fileNum = FREEFILE
OPEN fileName FOR BINARY AS #fileNum
PUT #fileNum, , work()
CLOSE #fileNum

IF _FILEEXISTS(fileName) THEN KILL fileName
PRINT "FAIL t268_negative_put_scalar_varstring"
SYSTEM
