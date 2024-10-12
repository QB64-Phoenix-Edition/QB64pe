'
' Duplicates the contents of one file into another
'
' Returns: 0 on success, 1 on error
FUNCTION CopyFile& (sourceFile$, destFile$)
    E = 0

    ON ERROR GOTO qberror_test
    dat$ = _READFILE$(sourceFile$)
    IF E = 0 THEN _WRITEFILE destFile$, dat$
    ON ERROR GOTO qberror

    CopyFile& = E
END FUNCTION

'
' Splits the filename from its path, and returns the path
'
' Returns: The path, or empty if no path
FUNCTION getfilepath$ (f$)
    FOR i = LEN(f$) TO 1 STEP -1
        a$ = MID$(f$, i, 1)
        IF a$ = "/" OR a$ = "\" THEN
            getfilepath$ = LEFT$(f$, i)
            EXIT FUNCTION
        END IF
    NEXT
    getfilepath$ = ""
END FUNCTION

'
' Checks if a filename has an extension on the end
'
' Returns: True if provided filename has an extension
FUNCTION FileHasExtension (f$)
    FOR i = LEN(f$) TO 1 STEP -1
        a = ASC(f$, i)
        IF a = 47 OR a = 92 THEN EXIT FOR
        IF a = 46 THEN FileHasExtension = -1: EXIT FUNCTION
    NEXT
END FUNCTION

'
' Strips the extension off of a filename
'
' Returns: Provided filename without extension on the end
FUNCTION RemoveFileExtension$ (f$)
    FOR i = LEN(f$) TO 1 STEP -1
        a = ASC(f$, i)
        IF a = 47 OR a = 92 THEN EXIT FOR
        IF a = 46 THEN RemoveFileExtension$ = LEFT$(f$, i - 1): EXIT FUNCTION
    NEXT
    RemoveFileExtension$ = f$
END FUNCTION

'
' Returns the extension on the end of a file name
'
' Returns "" if there is no extension
'
FUNCTION GetFileExtension$ (f$)
    FOR i = LEN(f$) TO 1 STEP -1
        a = ASC(f$, i)
        IF a = 47 OR a = 92 THEN EXIT FOR
        IF a = 46 THEN GetFileExtension$ = MID$(f$, i + 1): EXIT FUNCTION
    NEXT
    GetFileExtension$ = ""
END FUNCTION

'
' Fixes the provided filename and path to use the correct path separator
'
SUB PATH_SLASH_CORRECT (a$)
    IF os$ = "WIN" THEN
        FOR x = 1 TO LEN(a$)
            IF ASC(a$, x) = 47 THEN ASC(a$, x) = 92
        NEXT
    ELSE
        FOR x = 1 TO LEN(a$)
            IF ASC(a$, x) = 92 THEN ASC(a$, x) = 47
        NEXT
    END IF
END SUB

' Return a pathname where all "\" are correctly escaped
FUNCTION GetEscapedPath$ (path_name AS STRING)
    DIM buf AS STRING, z AS _UNSIGNED LONG, a AS _UNSIGNED _BYTE

    FOR z = 1 TO LEN(path_name)
        a = ASC(path_name, z)
        buf = buf + CHR$(a)
        IF a = 92 THEN buf = buf + "\"
    NEXT

    GetEscapedPath = buf
END FUNCTION

' Returns a path/file with single slashes only, effectively unescaping "\"
FUNCTION RemoveDoubleSlashes$ (f2$)
    f$ = f2$ 'avoid arg side effects

    DO 'sp% = 0 at function entry
        sp% = INSTR(sp% + 1, f$, "//")
        IF sp% > 0 THEN f$ = LEFT$(f$, sp% - 1) + MID$(f$, sp% + 1)
    LOOP UNTIL sp% = 0
    DO 'sp% = 0 again from 1st loop end
        sp% = INSTR(sp% + 1, f$, "\\")
        IF sp% > 0 THEN f$ = LEFT$(f$, sp% - 1) + MID$(f$, sp% + 1)
    LOOP UNTIL sp% = 0

    RemoveDoubleSlashes$ = f$
END FUNCTION

' Adds a trailing \ or / to a directory name if needed
FUNCTION FixDirectoryName$ (dir_name AS STRING)
    IF LEN(dir_name) > 0 AND RIGHT$(dir_name, 1) <> pathsep$ THEN
        FixDirectoryName = dir_name + pathsep$
    ELSE
        FixDirectoryName = dir_name
    END IF
END FUNCTION

