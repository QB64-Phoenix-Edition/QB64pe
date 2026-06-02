$Console:Only
OPTION _EXPLICIT

TYPE T269
    textItems(0 TO 1) _DynamicField AS STRING
    nums(0 TO 1) _DynamicField AS LONG
END TYPE

DIM fileName AS STRING
DIM fileNum AS INTEGER
REDIM work(0) AS T269

fileName = "t269_negative_get_dynamic_varstring_field.bin"
IF _FILEEXISTS(fileName) THEN KILL fileName

fileNum = FREEFILE
OPEN fileName FOR BINARY AS #fileNum
GET #fileNum, , work()
CLOSE #fileNum

IF _FILEEXISTS(fileName) THEN KILL fileName
PRINT "FAIL t269_negative_get_dynamic_varstring_field"
SYSTEM
