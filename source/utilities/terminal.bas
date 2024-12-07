' This function finds a usable terminal installed on Linux and provides the
' required configuration for it. This allows the terminal to display $CONSOLE
' programs and logging if requested.
'
' The user can replace the result with their own configuration, this is just a
' best effort to find a working default.
FUNCTION findWorkingTerminal$ ()
    DIM Exes$(7), Formats$(7)
    Exes$(1) = "gnome-terminal": Formats$(1) = "-- $$ $@"
    Exes$(2) = "konsole": Formats$(2) = "-e $$ $@"
    Exes$(3) = "lxterminal": Formats$(3) = "-e $$ $@"
    Exes$(4) = "mate-terminal": Formats$(4) = "-x $$ $@"
    Exes$(5) = "xfce4-terminal": Formats$(5) = "-x $$ $@"
    Exes$(6) = "urxvt": Formats$(6) = "-e $$ $@"
    Exes$(7) = "xterm": Formats$(7) = "-e $$ $@"

    FOR i = 1 TO UBOUND(Exes$)
        ret& = SHELL("command -v " + CHR$(34) + Exes$(i) + CHR$(34) + " >/dev/null 2>&1")

        IF ret& = 0 THEN
            findWorkingTerminal$ = Exes$(i) + " " + Formats$(i)
            EXIT FUNCTION
        END IF
    NEXT

    findWorkingTerminal$ = ""
END FUNCTION

SUB generateMacOSLogScript (exe AS STRING, handler AS STRING, scopes AS STRING, cmdstr AS STRING, script AS STRING)
    ON ERROR GOTO _NEWHANDLER qberror_test
    KILL script
    ON ERROR GOTO _LASTHANDLER

    _DELAY .01

    ff = FREEFILE
    OPEN script FOR OUTPUT AS #ff

    PRINT #ff, "export QB64PE_LOG_HANDLERS="; handler; _CHR_LF;
    PRINT #ff, "export QB64PE_LOG_SCOPES="; _CHR_QUOTE; scopes; _CHR_QUOTE; _CHR_LF;
    PRINT #ff, _CHR_QUOTE; exe; _CHR_QUOTE; " "; cmdstr; _CHR_LF;

    CLOSE #ff

    SHELL _HIDE "chmod +x " + _CHR_QUOTE + script + _CHR_QUOTE
END SUB

