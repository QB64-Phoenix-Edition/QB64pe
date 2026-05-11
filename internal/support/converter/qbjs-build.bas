$CONSOLE:ONLY
CONST ERROR_NO_NODEJS = 1, ERROR_NO_NETWORK = 2, ERROR_COMPILE_WARNINGS = 3
CONST ERROR_NO_SOURCE = 4, ERROR_MULTIPLE_SOURCE = 5, ERROR_INVALID_OPTION = 6, ERROR_INVALID_MODE = 7
OPTION _EXPLICIT
DIM SHARED AS STRING PORT, MODE, WARNING_FILE, PROGRAM_DIR, QB64_DIR
DIM SHARED AS INTEGER COMPILE_ONLY, NO_PROJECT_FILES, CLEAN
MODE = "auto"
PORT = "8080"

TYPE Dependency
    AS STRING src
    AS STRING dest
END TYPE
REDIM SHARED dependencies(0) AS Dependency
DIM SHARED AS STRING releaseTag, qbjsParentDir, qbjsDir, lastUpdate, destDir
DIM SHARED AS STRING sourceFilepath, filename, sourceDir
DIM SHARED AS INTEGER compileWarnings
PROGRAM_DIR = _CWD$
QB64_DIR = QB64ProgramDirectory

ParseArguments
CheckNodeJS
ReadConfig
GetCurrentRelease

' Initialize build paths
qbjsParentDir = _CWD$ + ".qbjs"
qbjsDir = qbjsParentDir + PathSeparator + "qbjs-" + MID$(releaseTag, 2)
filename = GetFilename(sourceFilepath)
sourceDir = GetParentPath(sourceFilepath)
IF sourceDir = PathSeparator THEN sourceDir = _STARTDIR$
IF MID$(sourceDir, LEN(sourceDir), 1) <> PathSeparator THEN sourceDir = sourceDir + PathSeparator
PRINT "Program Directory: " + PROGRAM_DIR
PRINT "QB64 Directory:    " + QB64_DIR
PRINT "Source Directory:  " + sourceDir
IF sourceDir = QB64_DIR THEN
    PRINT " - Copy project files disabled for QB64 program directory"
    NO_PROJECT_FILES = -1
END IF

DownloadQBJS
InitDependencies
CompileSource

IF COMPILE_ONLY THEN
    IF compileWarnings THEN SYSTEM ERROR_COMPILE_WARNINGS
    SYSTEM
END IF

CopyWebDependencies
CHDIR sourceDir
IF NOT NO_PROJECT_FILES THEN
    PRINT "Copying project files..."
    CHDIR sourceDir
    CopyProjectFiles ""
END IF
PRINT "Build complete."

DIM url AS STRING
url = "http://localhost:" + PORT + "/index.html"
StartWebserver url
PRINT "Launching page..."
LaunchURL url$
IF compileWarnings THEN SYSTEM ERROR_COMPILE_WARNINGS
SYSTEM

FUNCTION QB64ProgramDirectory$
    QB64ProgramDirectory$ = GetParentPath(GetParentPath(GetParentPath(GetParentPath(_CWD$)))) + PathSeparator
END FUNCTION

SUB ParseArguments
    DIM AS INTEGER i, scount
    DIM AS STRING arg, larg
    FOR i = 1 TO _COMMANDCOUNT
        arg = _TRIM$(COMMAND$(i))
        IF MID$(arg, 1, 1) <> "-" THEN
            sourceFilepath = arg
            scount = scount + 1
        ELSE
            larg = UCASE$(arg)
            IF larg = "-COMPILEONLY" THEN
                COMPILE_ONLY = -1
            ELSEIF larg = "-NOPROJECTFILES" THEN
                NO_PROJECT_FILES = -1
            ELSEIF larg = "-CLEAN" THEN
                CLEAN = -1
            ELSEIF LEN(larg) > 7 _ANDALSO MID$(larg, 1, 5) = "-PORT" THEN
                PORT = MID$(larg, 7, LEN(larg) - 6)
            ELSEIF LEN(larg) > 7 _ANDALSO MID$(larg, 1, 5) = "-MODE" THEN
                MODE = LCASE$(MID$(larg, 7, LEN(larg) - 6))
                IF MODE <> "auto" _ANDALSO MODE <> "play" THEN
                    PRINT "Invalid mode option: " + arg
                    PRINT "Valid modes are auto or play"
                    PrintUsage
                    SYSTEM ERROR_INVALID_MODE
                END IF
            ELSEIF LEN(larg) > 11 _ANDALSO MID$(larg, 1, 9) = "-WARNINGS" THEN
                WARNING_FILE = MID$(arg, 11, LEN(arg) - 10)
            ELSE
                PRINT "Invalid option: " + arg
                PrintUsage
                SYSTEM ERROR_INVALID_OPTION
            END IF
        END IF
    NEXT i
    IF sourceFilepath = "" THEN
        PRINT "No source file specified."
        PrintUsage
        SYSTEM ERROR_NO_SOURCE
    END IF
    IF scount > 1 THEN
        PRINT "More than one source file specified."
        PrintUsage
        SYSTEM ERROR_MULTIPLE_SOURCE
    END IF
END SUB

SUB PrintUsage
    PRINT
    PRINT "USAGE:"
    PRINT
    PRINT "qbjs-build source-filename.bas [-port:8080] [-mode:auto|play] [-compileOnly] [-noProjectFiles] [-clean]"
END SUB

SUB ReadConfig
    ' Read release version and last update information
    IF _FILEEXISTS(_CWD$ + ".qbjs-config") THEN
        OPEN _CWD$ + ".qbjs-config" FOR INPUT AS #1
        INPUT #1, releaseTag, lastUpdate
        CLOSE #1
    END IF
END SUB

SUB GetCurrentRelease
    DIM AS STRING text, searchStr
    DIM AS INTEGER sidx, eidx
    ' We probably don't need to check for new QBJS versions more than once per day
    IF lastUpdate <> DATE$ THEN
        ' Lookup current release
        PRINT "Checking current QBJS release version..."
        ' If the _OpenClient method supports setting the User-Agent header in the future,
        ' the following URL would be preferred to lookup this information:
        ' https://api.github.com/repos/boxgaming/qbjs/releases/latest
        IF DownloadFile("https://github.com/boxgaming/qbjs/releases/latest", ".qbjs_releases") = 200 THEN
            PRINT
            text = _READFILE$(".qbjs_releases")
            KILL ".qbjs_releases"
            searchStr = "/boxgaming/qbjs/releases/tag/"
            sidx = INSTR(text, searchStr) + LEN(searchStr)
            eidx = INSTR(sidx, text$, CHR$(34))
            releaseTag = MID$(text, sidx, eidx - sidx)
            ' Save the current release information
            OPEN _CWD$ + ".qbjs-config" FOR OUTPUT AS #1
            WRITE #1, releaseTag, DATE$
            CLOSE #1
        ELSEIF releaseTag = "" THEN
            PRINT "Unable to access QBJS repository, check network access."
            SYSTEM ERROR_NO_NETWORK
        END IF
    END IF
    PRINT "QBJS Web Build:    " + releaseTag
END SUB

SUB DownloadQBJS
    IF NOT _DIREXISTS(qbjsDir) THEN
        ' Install QBJS
        IF NOT _DIREXISTS(qbjsParentDir) THEN MKDIR qbjsParentDir

        PRINT "Downloading QBJS " + releaseTag + "...";
        PRINT DownloadFile("https://codeload.github.com/boxgaming/qbjs/zip/refs/tags/" + releaseTag, qbjsParentDir + PathSeparator + releaseTag + ".zip")
        PRINT "Download complete."
        PRINT "Unzipping QBJS..."
        $IF WINDOWS THEN
            SHELL "powershell " + Q$("$shell = New-Object -ComObject Shell.Application; $zipFile = $shell.NameSpace(\" + CHR$(34) + qbjsParentDir + PathSeparator + releaseTag + ".zip\" + CHR$(34) + "); $destination = $shell.NameSpace(\" + CHR$(34) + qbjsParentDir + "\" + CHR$(34) + "); $destination.CopyHere($zipFile.Items(), 20)")
        $ELSE
            SHELL "cd " + Q$(qbjsParentDir) + "; unzip " + releaseTag + ".zip"
        $END IF
        PRINT "Unzip complete."
        PRINT "Deleting zip."
        KILL qbjsParentDir + PathSeparator + releaseTag + ".zip"
    END IF
END SUB

SUB CompileSource
    DIM AS STRING warnings, warningFile
    destDir = sourceDir + "_web"

    IF CLEAN _ANDALSO _DIREXISTS(destDir) THEN
        $IF WINDOWS THEN
            SHELL "cmd.exe /c " + Q$("rmdir /s /q " + Q$(destDir))
        $ELSE
            SHELL "rm -rf " + Q$(destDir)
        $END IF
    END IF

    warningFile = PROGRAM_DIR + ".qbjs-warnings-" + _TRIM$(STR$(TIMER))
    IF WARNING_FILE <> "" THEN warningFile = WARNING_FILE
    PRINT "Warning File:      " + warningFile

    IF NOT _DIREXISTS(destDir) THEN MKDIR destDir
    CHDIR sourceDir

    IF _FILEEXISTS(warningFile) THEN KILL warningFile
    PRINT
    PRINT "Compiling source file: " + filename + "..."

    SHELL "node " + Q$(qbjsDir + PathSeparator + "qbc.js") + " " + Q$(sourceFilepath) + " " + Q$(destDir + PathSeparator + "program.js") + "> " + Q$(warningFile)
    warnings = ""
    IF _FILEEXISTS(warningFile) THEN warnings = _TRIM$(_READFILE$(warningFile))
    IF WARNING_FILE = "" THEN
        IF _FILEEXISTS(warningFile) THEN KILL warningFile
    END IF
    IF warnings = "" THEN
        warnings = "Compiled successfully with no errors or warnings"
        IF _FILEEXISTS(warningFile) THEN KILL warningFile
    ELSE
        compileWarnings = -1
    END IF
    PRINT "-----------------------------------------------------------------------------"
    PRINT warnings
    PRINT "-----------------------------------------------------------------------------"
    IF INSTR(warnings, "ERROR:") THEN SYSTEM ERROR_COMPILE_WARNINGS
END SUB

SUB CopyWebDependencies
    DIM i AS INTEGER
    DIM parent AS STRING

    PRINT "Copy web dependencies..."
    FOR i = 1 TO UBOUND(dependencies)
        parent = GetParentPath(dependencies(i).dest)
        IF parent <> PathSeparator THEN IF NOT _DIREXISTS(destDir + PathSeparator + parent) THEN MKDIR destDir + PathSeparator + parent
        $IF WINDOWS THEN
            SHELL "@echo off && cmd.exe /c " + Q$("copy /Y " + Q$(qbjsDir + PathSeparator + dependencies(i).src) + " " + Q$(destDir + PathSeparator + dependencies(i).dest)) + " > NUL"
        $ELSE
            SHELL "\cp -f " + Q$(qbjsDir + PathSeparator + dependencies(i).src) + " " + Q$(destDir + PathSeparator + dependencies(i).dest)
        $END IF
    NEXT i
END SUB

SUB CopyProjectFiles (path AS STRING)
    DIM i AS INTEGER
    DIM AS STRING file, ext

    REDIM dirs(0) AS STRING
    IF NOT _DIREXISTS(destDir + PathSeparator + path) THEN MKDIR destDir + PathSeparator + path
    file = _FILES$("")
    DO
        file = _FILES$
        IF _DIREXISTS(file) THEN
            IF file <> ".." + PathSeparator _ANDALSO file <> "." + PathSeparator _ANDALSO file <> "_web" + PathSeparator THEN
                i = UBOUND(dirs) + 1
                REDIM _PRESERVE dirs(i) AS STRING
                dirs(i) = file
            END IF
        ELSEIF file <> "" THEN
            ext = LCASE$(GetFileExtension$(file))
            IF ext <> "exe" _ANDALSO ext <> "bas" THEN
                $IF WINDOWS THEN
                    SHELL "@echo off && cmd.exe /c " + Q$("copy /Y " + Q$(file) + " " + Q$(destDir + PathSeparator + path) + " > NUL")
                $ELSE
                    SHELL "\cp -f " + Q$(file) + " " + Q$(destDir + PathSeparator + path)
                $END IF
            END IF
        END IF
    LOOP UNTIL file = ""

    FOR i = 1 TO UBOUND(dirs)
        CHDIR dirs(i)
        CopyProjectFiles path + dirs(i)
        CHDIR ".."
    NEXT i
END SUB

SUB StartWebserver (url AS STRING)
    DIM webServerDir AS STRING
    webServerDir = qbjsDir + PathSeparator + "tools" + PathSeparator
    IF NOT TestFile(url) THEN
        PRINT "Starting http server..."
        CHDIR destDir
        $IF WINDOWS THEN
            SHELL _DONTWAIT "start /min cmd.exe /c " + Q$("title QBJS Web Server && node " + EQ$(webServerDir + "qbjs-webserver.js") + " " + PORT)
        $ELSE
            SHELL _DONTWAIT "node " + Q$(webServerDir + "qbjs-webserver.js") + " " + PORT + " &"
        $END IF
    ELSE
        _WRITEFILE webServerDir + ".root-path-override", destDir
    END IF
END SUB

SUB CheckNodeJS
    DIM nodeVersion AS STRING
    SHELL "node --version > .qbjs-node-out 2>&1"
    nodeVersion = _TRIM$(_READFILE$(".qbjs-node-out"))
    nodeVersion = Replace$(nodeVersion, CHR$(10), "")
    nodeVersion = Replace$(nodeVersion, CHR$(13), "")
    KILL ".qbjs-node-out"
    IF MID$(nodeVersion, 1, 1) <> "v" THEN
        nodeVersion = ""
        PRINT "node.js not detected."
        PRINT "Please ensure that node.js is installed and is in the system path."
        LaunchURL "https://nodejs.org/en/download"
        SYSTEM ERROR_NO_NODEJS
    END IF

    PRINT "NodeJS Version:    " + nodeVersion
END SUB

FUNCTION GetFileExtension$ (filename AS STRING)
    DIM i AS INTEGER
    i = _INSTRREV(filename, ".")
    GetFileExtension = MID$(filename, i + 1)
END FUNCTION

FUNCTION Q$ (text AS STRING)
    Q$ = CHR$(34) + text + CHR$(34)
END FUNCTION

FUNCTION EQ$ (text AS STRING)
    EQ$ = "^" + CHR$(34) + text + "^" + CHR$(34)
END FUNCTION

FUNCTION TestFile (url AS STRING)
    DIM result AS INTEGER
    DIM h AS LONG
    h = _OPENCLIENT(url)
    IF h THEN
        IF _STATUSCODE(h) = 200 THEN result = -1
        CLOSE #h
    END IF
    TestFile = result
END FUNCTION

FUNCTION DownloadFile (url AS STRING, filename AS STRING)
    DIM h AS LONG, content AS STRING, s AS STRING
    DIM AS INTEGER statusCode

    h = _OPENCLIENT(url)

    IF h THEN
        OPEN filename FOR BINARY AS #1
        statusCode = _STATUSCODE(h)

        WHILE NOT EOF(h)
            _LIMIT 60
            GET #h, , s
            PUT #1, , s
            PRINT ".";
        WEND

        CLOSE #h
        CLOSE #1
    END IF

    DownloadFile = statusCode
END FUNCTION

SUB LaunchURL (url AS STRING)
    $IF WIN THEN
        SHELL _DONTWAIT _HIDE "start " + url
    $ELSEIF MAC THEN
        SHELL _DONTWAIT _HIDE "open " + url
    $ELSEIF LINUX THEN
        Shell _DONTWAIT _HIDE "xdg-open " + url
    $END IF
END SUB

FUNCTION GetFilename$ (filepath AS STRING)
    DIM s AS STRING, i AS INTEGER
    s = filepath
    s = Replace(s, "\", "/")
    i = _INSTRREV(s, "/")
    s = MID$(s, i + 1)
    GetFilename = s
END FUNCTION

FUNCTION GetParentPath$ (filepath AS STRING)
    DIM s AS STRING, i AS INTEGER
    s = filepath
    s = Replace(s, "\", "/")
    i = _INSTRREV(s, "/")
    s = MID$(s, 1, i - 1)
    s = Replace(s, "/", PathSeparator)
    IF s = "" THEN s = PathSeparator
    GetParentPath = s
END FUNCTION

FUNCTION PathSeparator$ ()
    $IF WINDOWS THEN
        PathSeparator = "\"
    $ELSE
        PathSeparator = "/"
    $END IF
END FUNCTION

FUNCTION Replace$ (s AS STRING, searchString AS STRING, newString AS STRING)
    DIM ns AS STRING
    DIM i AS INTEGER

    DIM slen AS INTEGER
    slen = LEN(searchString)

    FOR i = 1 TO LEN(s)
        IF MID$(s, i, slen) = searchString THEN
            ns = ns + newString
            i = i + slen - 1
        ELSE
            ns = ns + MID$(s, i, 1)
        END IF
    NEXT i

    Replace = ns
END FUNCTION

SUB AddDependency (src AS STRING, dest AS STRING)
    DIM i AS INTEGER
    i = UBOUND(dependencies) + 1
    REDIM _PRESERVE dependencies(i) AS Dependency
    $IF WINDOWS THEN
        src = Replace$(src, "/", "\")
        dest = Replace$(dest, "/", "\")
    $END IF
    dependencies(i).src = src
    dependencies(i).dest = dest
END SUB

SUB InitDependencies
    DIM AS STRING depfile, src, dest

    REDIM dependencies(0) AS Dependency
    AddDependency "export/" + MODE + ".html", "index.html"

    depfile = qbjsDir + PathSeparator + "export" + PathSeparator + "dependencies.txt"
    IF NOT _FILEEXISTS(depfile) THEN
        PRINT "Missing dependencies file:"
        PRINT depfile
    END IF

    OPEN depfile FOR INPUT AS #1
    INPUT #1, src, dest
    WHILE NOT EOF(1)
        AddDependency src, dest
        INPUT #1, src, dest
    WEND
    CLOSE #1
END SUB
