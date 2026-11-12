
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

'
' Writes buildflags.txt, a record of everything that decides how the generated
' C++ is compiled. The object files are named the same whatever they were built
' with, so the Makefile keys them on this: without it, turning SplitObjectFiles
' or OptimizeCppProgram off and on again would reuse an object compiled the
' other way.
'
SUB WriteBuildFlags (makedeps AS STRING, cxxflags AS STRING)
    DIM buf AS LONG

    buf = OpenBuffer&("O", tmpdir$ + "buildflags.txt")
    WriteBufLine buf, "deps:" + makedeps
    WriteBufLine buf, "cxxflags:" + cxxflags
    WriteBufLine buf, "bits:" + _TOSTR$(TARGET_BITS)
    IF SplitObjectFiles THEN
        WriteBufLine buf, "split:y"
    ELSE
        WriteBufLine buf, "split:n"
    END IF
END SUB

