$CONSOLE:ONLY
OPTION _EXPLICIT

ON ERROR GOTO test_failed

CHDIR _STARTDIR$

PRINT "Creating directory temp_dir"
MKDIR "temp_dir"

PRINT "Changing to directory temp_dir"
CHDIR "temp_dir"

PRINT "Changing to parent directory"
CHDIR ".."

PRINT "Renaming temp_dir to dummy_dir"
NAME "temp_dir" AS "dummy_dir"

PRINT "_DIREXISTS(dummy_dir):"; _DIREXISTS("./dummy_dir")

PRINT "Creating a temporary file inside dummy_dir"
DIM fileName AS STRING: fileName = CreateDummyFile("./dummy_dir/", ".tmp")

PRINT "_FILEEXISTS(fileName):"; _FILEEXISTS(fileName)

PRINT "Deleting fileName"
KILL fileName

PRINT "Creating 10 dummy files inside dummy_dir"
DIM i AS LONG: FOR i = 0 TO 9
    fileName = CreateDummyFile("./dummy_dir/", ".tmp")
NEXT i

' Start _FILES$ test
CHDIR "dummy_dir"

fileName = CreateDummyFile("./", "")

i = 0
DIM dirEntry AS STRING: dirEntry = _FILES$("") ' should count 13 entries

DO WHILE LEN(dirEntry) > 0
    i = i + 1

    dirEntry = _FILES$
LOOP

PRINT "Counted"; i; "entries."

i = 0
dirEntry = _FILES$("*.*") ' should count 12 entries

DO WHILE LEN(dirEntry) > 0
    i = i + 1

    dirEntry = _FILES$
LOOP

PRINT "Counted"; i; "entries."

KILL fileName

CHDIR ".."
' End _FILES$ test

PRINT "Deleting all 10 dummy files"
KILL "./dummy_dir/*.tmp"

PRINT "Deleting dummy_dir"
RMDIR "dummy_dir"

SYSTEM

test_failed:
PRINT "Test failed!"
SYSTEM 1


FUNCTION CreateDummyFile$ (directory AS STRING, extension AS STRING)
    DO
        DIM fileName AS STRING: fileName = directory + LTRIM$(STR$(100! * (TIMER + RND))) + extension
    LOOP WHILE _FILEEXISTS(fileName)

    DIM h AS LONG: h = FREEFILE

    OPEN fileName FOR OUTPUT AS h
    PRINT #h, "Delete me!"
    CLOSE h

    CreateDummyFile = fileName
END FUNCTION
