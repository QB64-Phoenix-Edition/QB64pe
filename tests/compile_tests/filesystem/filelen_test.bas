$CONSOLE:ONLY
OPTION _EXPLICIT

ON ERROR GOTO test_failed

CHDIR _STARTDIR$

PRINT "Test 1: Basic file size"
_WRITEFILE "testfile.txt", "Hello, World!" + _STR_CRLF

DIM fileSize AS _UNSIGNED _INTEGER64: fileSize = _FILELEN("testfile.txt")
PRINT "File size:"; fileSize; "bytes"
IF fileSize = 15 THEN
    PRINT "Test 1 passed"
ELSE
    PRINT "Test 1 failed: expected 15, got"; fileSize
    SYSTEM 1
END IF

KILL "testfile.txt"

PRINT "Test 2: Non-existent file"
ON ERROR GOTO ignore_error
DIM missingSize AS _UNSIGNED _INTEGER64: missingSize = _FILELEN("nonexistent_file.txt")
PRINT "Non-existent file size:"; missingSize
IF missingSize = 0 THEN
    PRINT "Test 2 passed"
ELSE
    PRINT "Test 2 failed: expected 0, got"; missingSize
    SYSTEM 1
END IF
ON ERROR GOTO test_failed
GOTO continue_test

ignore_error:
RESUME NEXT

continue_test:

PRINT "Test 3: Empty file"
_WRITEFILE "empty.txt", ""

DIM emptySize AS _UNSIGNED _INTEGER64: emptySize = _FILELEN("empty.txt")
PRINT "Empty file size:"; emptySize; "bytes"
IF emptySize = 0 THEN
    PRINT "Test 3 passed"
ELSE
    PRINT "Test 3 failed: expected 0, got"; emptySize
    SYSTEM 1
END IF

KILL "empty.txt"

PRINT "Test 4: Large file"
DIM largeContent AS STRING: largeContent = ""
DIM i AS LONG: FOR i = 1 TO 1000
    largeContent = largeContent + "This is a test line with some data" + _STR_CRLF
NEXT i
_WRITEFILE "large.txt", largeContent

DIM largeSize AS _UNSIGNED _INTEGER64: largeSize = _FILELEN("large.txt")
PRINT "Large file size:"; largeSize; "bytes"
IF largeSize > 0 THEN
    PRINT "Test 4 passed"
ELSE
    PRINT "Test 4 failed: expected > 0, got"; largeSize
    SYSTEM 1
END IF

KILL "large.txt"

PRINT "All tests passed!"
SYSTEM

test_failed:
PRINT "Test failed!"
SYSTEM 1
