$CONSOLE:ONLY
OPTION _EXPLICIT

ON ERROR GOTO test_failed

CHDIR _STARTDIR$

PRINT "Test 1: Normal file"
_WRITEFILE "normal.txt", "Hello, World!" + _STR_CRLF

DIM attrs AS LONG: attrs = _GETATTR("normal.txt")
IF attrs = 0 OR attrs = 32 THEN
    PRINT "Test 1 passed"
ELSE
    PRINT "Test 1 failed: expected 0 or 32, got"; attrs
    SYSTEM 1
END IF

KILL "normal.txt"

PRINT "Test 2: Directory"
MKDIR "testdir"
attrs = _GETATTR("testdir")
IF (attrs AND 16) THEN
    PRINT "Test 2 passed"
ELSE
    PRINT "Test 2 failed: expected bit 16 set, got"; attrs
    SYSTEM 1
END IF

RMDIR "testdir"

PRINT "Test 3: Non-existent path"
ON ERROR GOTO ignore_error
attrs = _GETATTR("nonexistent_file.txt")
IF attrs = 0 THEN
    PRINT "Test 3 passed"
ELSE
    PRINT "Test 3 failed: expected 0, got"; attrs
    SYSTEM 1
END IF
ON ERROR GOTO test_failed
GOTO continue_test

ignore_error:
RESUME NEXT

continue_test:

PRINT "Test 4: Bitwise AND check"
MKDIR "testdir"
attrs = _GETATTR("testdir")
IF (attrs AND 16) THEN
    PRINT "Test 4 passed - directory detected with AND"
ELSE
    PRINT "Test 4 failed"
    RMDIR "testdir"
    SYSTEM 1
END IF

RMDIR "testdir"

PRINT "All tests passed!"
SYSTEM

test_failed:
PRINT "Test failed!"
SYSTEM 1
