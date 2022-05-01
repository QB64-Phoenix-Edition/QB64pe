
'
' Duplicates the contents of one file into another
'
' Returns: 0 on success, 1 on error
FUNCTION CopyFile& (sourceFile$, destFile$)
    DIM sourcefileNo, destFileNo
    DIM fileLength AS _INTEGER64

    E = 0
    sourceFileNo = FREEFILE
    OPEN sourceFile$ FOR BINARY as #sourceFileNo
    if E = 1 THEN GOTO errorCleanup

    fileLength = LOF(sourceFileNo)

    destFileNo = FREEFILE
    OPEN destFile$ FOR BINARY as #destFileNo
    if E = 1 THEN GOTO errorCleanup

    ' Read the file in one go
    buffer$ = SPACE$(fileLength)

    GET #sourceFileNo, , buffer$
    PUT #destFileNo, , buffer$

errorCleanup:
    IF sourceFileNo <> 0 THEN CLOSE #sourceFileNo
    IF destFileNo <> 0 THEN CLOSE #destFileNo

    CopyFile& = E
END FUNCTION
