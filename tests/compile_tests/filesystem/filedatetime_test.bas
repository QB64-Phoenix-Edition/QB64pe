$CONSOLE:ONLY
OPTION _EXPLICIT

ON ERROR GOTO test_failed

CHDIR _STARTDIR$

PRINT "Test 1: Normal file"
_WRITEFILE "testfile.txt", "Hello, World!" + _STR_CRLF

IF LEN(_FILEDATETIME$("testfile.txt")) = 19 THEN
    PRINT "Test 1 passed"
ELSE
    PRINT "Test 1 failed"
    SYSTEM 1
END IF

KILL "testfile.txt"

PRINT "Test 2: Non-existent path"
ON ERROR GOTO ignore_error
IF _FILEDATETIME$("nonexistent_file.txt") = "" THEN
    PRINT "Test 2 passed"
ELSE
    PRINT "Test 2 failed"
    SYSTEM 1
END IF
ON ERROR GOTO test_failed
GOTO continue_test

ignore_error:
RESUME NEXT

continue_test:

PRINT "All tests passed!"
SYSTEM

test_failed:
PRINT "Test failed!"
SYSTEM 1
