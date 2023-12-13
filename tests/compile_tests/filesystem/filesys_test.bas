$CONSOLE:ONLY
OPTION _EXPLICIT

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
DIM fileName AS STRING: fileName = CreateDummyFile$("./dummy_dir/")

PRINT "_FILEEXISTS(fileName):"; _FILEEXISTS(fileName)

PRINT "Deleting fileName"
KILL fileName

PRINT "Creating 10 dummy files inside dummy_dir"
DIM i AS LONG: FOR i = 0 TO 9
    fileName = CreateDummyFile$("./dummy_dir/")
NEXT i

PRINT "Deleting all 10 dummy files"
KILL "./dummy_dir/*.tmp"

PRINT "Deleting dummy_dir"
RMDIR "dummy_dir"

SYSTEM

FUNCTION CreateDummyFile$ (directory AS STRING)
    DO
        DIM fileName AS STRING: fileName = LTRIM$(STR$(100! * (TIMER + RND))) + ".tmp"
    LOOP WHILE _FILEEXISTS(directory + fileName)

    fileName = directory + fileName

    DIM h AS LONG: h = FREEFILE

    OPEN fileName FOR OUTPUT AS h
    PRINT #h, "Delete me!"
    CLOSE h

    CreateDummyFile = fileName
END FUNCTION
