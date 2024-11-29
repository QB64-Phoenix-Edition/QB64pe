$CONSOLE:ONLY
OPTION _EXPLICIT

DIM myArray(1 TO 10) AS LONG
DIM i AS LONG

FOR i = LBOUND(myArray) TO UBOUND(myArray)
    myArray(i) = 2 ^ i
NEXT i

PrintArray myArray()

DIM arrMem AS _MEM: arrMem = _MEM(myArray())

FOR i = 0 TO (arrMem.SIZE \ _SIZE_OF_LONG) - 1
    _MEMPUT arrMem, arrMem.OFFSET + (i * _SIZE_OF_LONG), i + 1 AS LONG
NEXT i

_MEMFREE arrMem

PrintArray myArray()

SYSTEM

SUB PrintArray (arr() AS LONG)
    DIM i AS LONG

    PRINT "Array("; LBOUND(arr); "To"; UBOUND(arr); ") = {";
    FOR i = LBOUND(arr) TO UBOUND(arr)
        PRINT arr(i); _IIF(i < UBOUND(arr), ",", "");
    NEXT i
    PRINT "}"
END SUB
