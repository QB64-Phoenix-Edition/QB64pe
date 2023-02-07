
'
' Removes all the temporary build files (Various .o, .a, and ./internal/temp, among other things)
'
SUB PurgeTemporaryBuildFiles (os AS STRING, mac AS LONG)
    make$ = GetMakeExecutable$

    IF os = "WIN" THEN
        SHELL _HIDE "cmd /c " + make$ + " OS=win clean"
    ELSEIF os = "LNX" THEN
        IF mac THEN
            SHELL _HIDE make$ + " OS=osx clean"
        ELSE
            SHELL _HIDE make$ + " OS=lnx clean"
        END IF
    END IF
END SUB

'
' Returns the make executable to use, with path if necessary
'
FUNCTION GetMakeExecutable$ ()
    IF os$ = "WIN" THEN
        GetMakeExecutable$ = "internal\c\c_compiler\bin\mingw32-make.exe"
    ELSE
        GetMakeExecutable$ = "make"
    END IF
END FUNCTION

FUNCTION MakeNMOutputFilename$ (libfile AS STRING, dynamic As Long)
    If dynamic Then dyn$ = "_dynamic" Else dyn$ = ""

    MakeNMOutputFilename$ = tmpdir$ + "nm_output_" + StrReplace$(StrReplace$(libfile, pathsep$, "."), ":", ".") + dyn$ + ".txt"
END FUNCTION
