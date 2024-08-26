DIM SHARED Version AS STRING
DIM SHARED IsCiVersion AS _BYTE

Version$ = "3.14.1"
$VERSIONINFO:FILEVERSION#=3,14,1,0
$VERSIONINFO:PRODUCTVERSION#=3,14,1,0

' If ./internal/version.txt exist, then this is some kind of CI build with a label
IF _FILEEXISTS("internal/version.txt") THEN
    versionfile = FREEFILE
    OPEN "internal/version.txt" FOR INPUT AS #versionfile

    LINE INPUT #versionfile, VersionLabel$
    Version$ = Version$ + VersionLabel$

    IF VersionLabel$ <> "" AND VersionLabel$ <> "-UNKNOWN" THEN
        IsCiVersion = -1
    END IF

    CLOSE #versionfile
END IF
