$If VERSION_BAS = UNDEFINED Then
    Dim Shared Version As String
    Dim Shared DevChannel As String
    Dim Shared AutoBuildMsg As String

    Version$ = "v0.1 -- Phoenix Edition (First Release) "
    DevChannel$ = "Development Build"
    If _FileExists("internal/version.txt") Then
        versionfile = FreeFile
        Open "internal/version.txt" For Input As #versionfile
        Line Input #versionfile, AutoBuildMsg
        AutoBuildMsg = Left$(_Trim$(AutoBuildMsg), 16) 'From git 1234567
        If Left$(AutoBuildMsg, 9) <> "From git " Then AutoBuildMsg = ""
        Close #versionfile
    End If
$End If
$Let VERSION_BAS = TRUE
