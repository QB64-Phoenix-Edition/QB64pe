
'
' Removes all the temporary build files (Various .o, .a, and ./internal/temp, among other things)
'
SUB PurgeTemporaryBuildFiles (os AS STRING, mac AS LONG)
    IF os = "WIN" THEN
        SHELL _HIDE "cmd /c " + GetMakeExecutable$ + " OS=win clean"
    ELSEIF os = "LNX" THEN
        IF mac THEN
            SHELL _HIDE GetMakeExecutable$ + " OS=osx clean"
        ELSE
            SHELL _HIDE GetMakeExecutable$ + " OS=lnx clean"
        END IF
    END IF
END SUB

'
' Returns the make executable to use, with path if necessary
'
FUNCTION GetMakeExecutable$ ()
    IF os$ = "WIN" THEN
        GetMakeExecutable$ = GetCompilerPath$ + "mingw32-make.exe"
    ELSE
        GetMakeExecutable$ = "make"
    END IF
END FUNCTION

FUNCTION GetCompilerPath$
    GetCompilerPath$ = _IIF(UseSystemMinGW, "", "internal\c\c_compiler\bin\")
END FUNCTION

FUNCTION MakeNMOutputFilename$ (libfile AS STRING, dynamic AS LONG)
    IF dynamic THEN dyn$ = "_dynamic" ELSE dyn$ = ""

    MakeNMOutputFilename$ = tmpdir$ + "nm_output_" + StrReplace$(StrReplace$(libfile, pathsep$, "."), ":", ".") + dyn$ + ".txt"
END FUNCTION

