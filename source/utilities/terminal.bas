' This function finds a usable terminal installed on Linux and provides the
' required configuration for it. This allows the terminal to display $CONSOLE
' programs and logging if requested.
'
' The user can replace the result with their own configuration, this is just a
' best effort to find a working default.
FUNCTION findWorkingTerminal$ ()
    DIM Exes$(8), Formats$(8)
    Exes$(1) = "gnome-terminal": Formats$(1) = "-- $$ $@"
    Exes$(2) = "konsole": Formats$(2) = "-e $$ $@"
    Exes$(3) = "lxterminal": Formats$(3) = "-e $$ $@"
    Exes$(4) = "mate-terminal": Formats$(4) = "-x $$ $@"
    Exes$(5) = "xfce4-terminal": Formats$(5) = "-x $$ $@"
    Exes$(6) = "urxvt": Formats$(6) = "-e $$ $@"
    Exes$(7) = "xterm": Formats$(7) = "-e $$ $@"
    Exes$(8) = "ptyxis": Formats$(8) = "-- $$ $@"

    FOR i = 1 TO UBOUND(Exes$)
        ret& = SHELL("command -v " + CHR$(34) + Exes$(i) + CHR$(34) + " >/dev/null 2>&1")

        IF ret& = 0 THEN
            findWorkingTerminal$ = Exes$(i) + " " + Formats$(i)
            EXIT FUNCTION
        END IF
    NEXT

    findWorkingTerminal$ = ""
END FUNCTION

SUB generateMacOSLogScript (exe AS STRING, cmdstr AS STRING, script AS STRING)
    ON ERROR GOTO _NEWHANDLER qberror_test
    KILL script
    ON ERROR GOTO _LASTHANDLER

    _DELAY .05

    ff = FREEFILE
    OPEN script FOR OUTPUT AS #ff

    IF LoggingEnabled THEN
        PRINT #ff, "export QB64PE_LOG_LEVEL="; _CHR_QUOTE; LogMinLevel$; _CHR_QUOTE; _CHR_LF;
        PRINT #ff, "export QB64PE_LOG_SCOPES="; _CHR_QUOTE; LogScopes$; _CHR_QUOTE; _CHR_LF;
        PRINT #ff, "export QB64PE_LOG_HANDLERS="; _CHR_QUOTE; LogHandlers$; _CHR_QUOTE; _CHR_LF;
        PRINT #ff, "export QB64PE_LOG_FILE_PATH="; _CHR_QUOTE; LogFileName$; _CHR_QUOTE; _CHR_LF;
    ELSE
        PRINT #ff, "export QB64PE_LOG_LEVEL="; _CHR_LF;
        PRINT #ff, "export QB64PE_LOG_SCOPES="; _CHR_LF;
        PRINT #ff, "export QB64PE_LOG_HANDLERS="; _CHR_LF;
        PRINT #ff, "export QB64PE_LOG_FILE_PATH="; _CHR_LF;
    END IF
    PRINT #ff, _CHR_QUOTE; exe; _CHR_QUOTE; " "; cmdstr; _CHR_LF;

    CLOSE #ff

    SHELL _HIDE "chmod +x " + _CHR_QUOTE + script + _CHR_QUOTE
END SUB

