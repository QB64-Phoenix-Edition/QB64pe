
'
' Duplicates the contents of one file into another
'
' Returns: 0 on success, 1 on error
FUNCTION CopyFile& (sourceFile$, destFile$)
    DIM sourceFileNo, destFileNo
    DIM fileLength AS _INTEGER64

    E = 0
    sourceFileNo = FREEFILE
    OPEN sourceFile$ FOR BINARY AS #sourceFileNo
    IF E = 1 THEN GOTO errorCleanup

    fileLength = LOF(sourceFileNo)

    destFileNo = FREEFILE
    OPEN destFile$ FOR OUTPUT AS #destFileNo: CLOSE #destFileNo 'create and blank any existing file with the dest name.
    OPEN destFile$ FOR BINARY AS #destFileNo
    IF E = 1 THEN GOTO errorCleanup

    ' Read the file in one go
    buffer$ = SPACE$(fileLength)

    GET #sourceFileNo, , buffer$
    PUT #destFileNo, , buffer$

    errorCleanup:
    IF sourceFileNo <> 0 THEN CLOSE #sourceFileNo
    IF destFileNo <> 0 THEN CLOSE #destFileNo

    CopyFile& = E
END FUNCTION

'
' Convert a file into an C-Array incl. read function and append it
' to the file 'internal\temp\embedded.cpp'
'
' Inputs: sourcefile spec, unique handle (case sensitive)
' Return: 0 = normal embed, 1 = packed embed (DEPENDENCY_ZLIB required)
FUNCTION ConvertFileToCArray% (file$, handle$)
    '--- read file contents ---
    sff% = FREEFILE
    OPEN "B", #sff%, file$
    filedata$ = SPACE$(LOF(sff%))
    GET #sff%, , filedata$
    CLOSE #sff%
    '--- try to compress ---
    compdata$ = _DEFLATE$(filedata$)
    IF LEN(compdata$) < (LEN(filedata$) * 0.8) THEN
        tmpfile$ = tmpdir$ + "embed.bin"
        OPEN "O", #sff%, tmpfile$: CLOSE #sff%
        OPEN "B", #sff%, tmpfile$: PUT #sff%, , compdata$: CLOSE #sff%
        packed% = 1
        OPEN "B", #sff%, tmpfile$
    ELSE
        packed% = 0
        OPEN "B", #sff%, file$
    END IF
    '--- init variables ---
    fl& = LOF(sff%)
    cntL& = INT(fl& / 32)
    cntV& = INT(cntL& / 8180)
    cntB& = (fl& - (cntL& * 32))
    '--- create C-Array ---
    dff% = FREEFILE
    OPEN "A", #dff%, tmpdir$ + "embedded.cpp"
    '--- process LONGs ---
    tmpI$ = SPACE$(32)
    FOR vc& = 0 TO cntV&
        IF vc& = cntV& THEN numL& = (cntL& MOD 8180): ELSE numL& = 8180
        PRINT #dff%, "static const uint32_t "; handle$; "L"; LTRIM$(STR$(vc&)); "[] = {"
        PRINT #dff%, "    "; LTRIM$(STR$(numL& * 8)); ","
        FOR z& = 1 TO numL&
            GET #sff%, , tmpI$: offI% = 1
            tmpO$ = "    " + STRING$(88, ","): offO% = 5
            DO
                tmpL& = CVL(MID$(tmpI$, offI%, 4)): offI% = offI% + 4
                MID$(tmpO$, offO%, 10) = "0x" + RIGHT$("00000000" + HEX$(tmpL&), 8)
                offO% = offO% + 11
            LOOP UNTIL offO% > 92
            IF z& < numL& THEN PRINT #dff%, tmpO$: ELSE PRINT #dff%, LEFT$(tmpO$, 91)
        NEXT z&
        PRINT #dff%, "};"
        PRINT #dff%, ""
    NEXT vc&
    '--- process remaining BYTEs ---
    IF cntB& > 0 THEN
        PRINT #dff%, "static const uint8_t "; handle$; "B[] = {"
        PRINT #dff%, "    "; LTRIM$(STR$(cntB&)); ","
        PRINT #dff%, "    ";
        FOR x% = 1 TO cntB&
            GET #sff%, , tmpB%%
            PRINT #dff%, "0x" + RIGHT$("00" + HEX$(tmpB%%), 2);
            IF x% <> 16 THEN
                IF x% <> cntB& THEN PRINT #dff%, ",";
            ELSE
                IF x% <> cntB& THEN
                    PRINT #dff%, ","
                    PRINT #dff%, "    ";
                END IF
            END IF
        NEXT x%
        PRINT #dff%, ""
        PRINT #dff%, "};"
        PRINT #dff%, ""
    END IF
    '--- make a read function ---
    PRINT #dff%, "qbs *GetArrayData_"; handle$; "(void)"
    PRINT #dff%, "{"
    PRINT #dff%, "    qbs  *data = qbs_new("; LTRIM$(STR$(fl&)); ", 1);"
    PRINT #dff%, "    char *buff = (char*) data -> chr;"
    PRINT #dff%, ""
    FOR vc& = 0 TO cntV&
        PRINT #dff%, "    memcpy(buff, &"; handle$; "L"; LTRIM$(STR$(vc&)); "[1], "; handle$; "L"; LTRIM$(STR$(vc&)); "[0] << 2);"
        IF vc& < cntV& OR cntB& > 0 THEN
            PRINT #dff%, "    buff += ("; handle$; "L"; LTRIM$(STR$(vc&)); "[0] << 2);"
        END IF
    NEXT vc&
    IF cntB& > 0 THEN
        PRINT #dff%, "    memcpy(buff, &"; handle$; "B[1], "; handle$; "B[0]);"
    END IF
    PRINT #dff%, ""
    IF packed% THEN
        PRINT #dff%, "    return func__inflate(data, "; LTRIM$(STR$(LEN(filedata$))); ", 1);"
    ELSE
        PRINT #dff%, "    return data;"
    END IF
    PRINT #dff%, "}"
    PRINT #dff%, ""
    '--- ending ---
    CLOSE #dff%
    CLOSE #sff%
    ConvertFileToCArray% = packed%
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

' Adds a trailing \ or / to a directory name if needed
FUNCTION FixDirectoryName$ (dir_name AS STRING)
    IF LEN(dir_name) > 0 AND RIGHT$(dir_name, 1) <> pathsep$ THEN
        FixDirectoryName = dir_name + pathsep$
    ELSE
        FixDirectoryName = dir_name
    END IF
END FUNCTION

