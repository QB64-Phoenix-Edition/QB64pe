DIM SHARED Version AS STRING
DIM SHARED IsCiVersion AS _BYTE

' Attention !!
' As a temporary solution please also update the version number in the
' user agent string in function libqb_http_open() in the source file here:
' ... internal\c\libqb\src\qb_http.cpp
' As soon as Github Issue #619 is properly implemented we can remove this
' temporary solution on the C/C++ level and rather create the user agent
' string as needed on the BASIC level by incorporating the version number
' string defined below.

Version$ = "4.4.0"
$VERSIONINFO:FILEVERSION#=4,4,0,0
$VERSIONINFO:PRODUCTVERSION#=4,4,0,0

' If ./internal/version.txt exist, then this is some kind of CI build with a label
IF _FILEEXISTS("internal/version.txt") THEN
    versionfile = FREEFILE
    OPEN "internal/version.txt" FOR INPUT AS #versionfile

    LINE INPUT #versionfile, VersionLabel$
    Version$ = Version$ + VersionLabel$

    IF VersionLabel$ <> "" AND VersionLabel$ <> "-UNKNOWN" THEN
        IsCiVersion = _TRUE
    END IF

    CLOSE #versionfile
END IF

