'Add an entry in the top position of the specified history. If the entry
'already exists in the history, then it's just moved back to top again.
'---------------------------------------------------------------------
SUB AddToHistory (which$, entry$)
    SELECT CASE which$
        CASE "RECENT"
            e$ = RemoveDoubleSlashes$(entry$)
            bh% = OpenBuffer%("I", RecentFile$) 'load and/or set pos (back) to start
        CASE "SEARCH"
            e$ = entry$
            bh% = OpenBuffer%("I", SearchedFile$) 'load and/or set pos (back) to start
    END SELECT
    lc% = 0: ue$ = UCASE$(e$)
    WHILE NOT EndOfBuf%(bh%)
        be$ = ReadBufLine$(bh%): lc% = lc% + 1
        IF UCASE$(be$) = ue$ OR lc% = 100 THEN 'already known or limit reached?
            nul& = SeekBuf&(bh%, -LEN(BufEolSeq$(bh%)), SBM_BufCurrent) 'back to prev (just read) line
            nul& = SeekBuf&(bh%, 0, SBM_LineStart) 'and then ahead to the start of it
            DeleteBufLine bh%
            EXIT WHILE
        END IF
    WEND
    nul& = SeekBuf&(bh%, 0, SBM_BufStart) 'rewind
    WriteBufLine bh%, e$ 'put new (or known) in 1st position (again)
    UpdateHistory which$
END SUB

'Helper functions required for file locking on Linux and Mac systems.
'---------------------------------------------------------------------
DECLARE LIBRARY "source/ide/ide_filelock"
    FUNCTION LockGlobalFileAccess& (filename$) 'add CHR$(0)
    SUB ReleaseGlobalFileAccess ()
END DECLARE

'Update the specified history file on disk (i.e. save the current history
'buffer) and place a sync signal for all other running IDE instances.
'---------------------------------------------------------------------
SUB UpdateHistory (which$)
    ON ERROR GOTO qberror_test
    SELECT CASE which$
        CASE "RECENT": hist$ = RecentFile$
        CASE "SEARCH": hist$ = SearchedFile$
    END SELECT
    RANDOMIZE TIMER: wlim% = INT(RND(1) * 16) + 5 'vary wait loop to equalize chances
    hfn% = FREEFILE
    DO
        _LIMIT wlim%
        E = LockGlobalFileAccess&(hist$ + FlagExt$ + CHR$(0))
        IF E = 0 THEN
            OPEN hist$ + FlagExt$ FOR BINARY LOCK WRITE AS #hfn%
        END IF
    LOOP UNTIL E = 0
    dat$ = STRING$(1001, 0): synced%% = -1 'reset signal file
    PUT #hfn%, 1, dat$: PUT #hfn%, tempfolderindex, synced%%
    WriteBuffers hist$ 'save current history to disk
    IF which$ = "RECENT" THEN IdeMakeFileMenu LEFT$(menu$(1, FileMenuExportAs), 1) <> "~"
    CLOSE #hfn%: ReleaseGlobalFileAccess
    ON ERROR GOTO qberror
END SUB

'Try to synchronize the specified history buffer with the contents of
'the disk based history file in a non-blocking manner. If this is not
'currently possible, then do nothing, the function is called again in
'the next IDE input loop until the sync was successful.
'---------------------------------------------------------------------
SUB SyncHistory (which$)
    ON ERROR GOTO qberror_test
    SELECT CASE which$
        CASE "RECENT": hist$ = RecentFile$
        CASE "SEARCH": hist$ = SearchedFile$
    END SELECT
    hfn% = FREEFILE
    E = LockGlobalFileAccess&(hist$ + FlagExt$ + CHR$(0))
    IF E = 0 THEN
        OPEN hist$ + FlagExt$ FOR BINARY LOCK WRITE AS #hfn%
        IF E = 0 THEN
            GET #hfn%, tempfolderindex, synced%%
            IF synced%% = 0 THEN 'login this instance and sync
                GET #hfn%, 1000, instUseCnt%
                synced%% = -1: instUseCnt% = instUseCnt% + 1
                PUT #hfn%, tempfolderindex, synced%%: PUT #hfn%, 1000, instUseCnt%
                ClearBuffers hist$ 'force reload on next access
                IF which$ = "RECENT" THEN IdeMakeFileMenu LEFT$(menu$(1, FileMenuExportAs), 1) <> "~"
            END IF
            CLOSE #hfn%
        END IF
        ReleaseGlobalFileAccess
    END IF
    ON ERROR GOTO qberror
END SUB

'Log the calling IDE instance out from sync'ing the specified history.
'When only one running instance remains, then the sync signal for the
'respective history is removed.
'---------------------------------------------------------------------
SUB ReleaseHistory (which$)
    ON ERROR GOTO qberror_test
    SELECT CASE which$
        CASE "RECENT": sigf$ = RecentFile$ + FlagExt$
        CASE "SEARCH": sigf$ = SearchedFile$ + FlagExt$
    END SELECT
    IF _FILEEXISTS(sigf$) THEN
        RANDOMIZE TIMER: wlim% = INT(RND(1) * 16) + 5 'vary wait loop to equalize chances
        sfn% = FREEFILE: instUseCnt% = 1 'default = no KILL (below)
        DO
            _LIMIT wlim%
            IF NOT _FILEEXISTS(sigf$) GOTO rhDone 'finished by another instance
            E = LockGlobalFileAccess&(sigf$ + CHR$(0))
            IF E = 0 THEN
                OPEN sigf$ FOR BINARY LOCK WRITE AS #sfn%
            END IF
        LOOP UNTIL E = 0
        GET #sfn%, tempfolderindex, synced%%
        IF synced%% <> 0 THEN 'logout this instance
            GET #sfn%, 1000, instUseCnt%
            synced%% = 0: instUseCnt% = instUseCnt% - 1
            PUT #sfn%, tempfolderindex, synced%%: PUT #sfn%, 1000, instUseCnt%
        END IF
        CLOSE #sfn%: ReleaseGlobalFileAccess
        IF instUseCnt% < 1 THEN KILL sigf$ 'remove signal file on last instance
    END IF
    rhDone:
    ON ERROR GOTO qberror
END SUB

'A simple "Are you sure" type yes/no messagebox for cleanup operations.
'---------------------------------------------------------------------
FUNCTION AskClearHistory$ (which$)
    SELECT CASE which$
        CASE "RECENT": t$ = "Clear recent files"
        CASE "SEARCH": t$ = "Clear search history"
    END SELECT
    result = idemessagebox(t$, "This cannot be undone. Proceed?", "#Yes;#No")
    IF result = 1 THEN AskClearHistory$ = "Y" ELSE AskClearHistory$ = "N"
END FUNCTION

'Will remove all broken links from the recent files history.
'---------------------------------------------------------------------
SUB CleanUpRecentList
    bh% = OpenBuffer%("I", RecentFile$) 'load and/or set pos (back) to start
    allOk% = -1 'let's assume the list is OK
    WHILE NOT EndOfBuf%(bh%)
        IF NOT _FILEEXISTS(ReadBufLine$(bh%)) THEN 'accessible?
            nul& = SeekBuf&(bh%, -LEN(BufEolSeq$(bh%)), SBM_BufCurrent) 'back to prev (just read) line
            nul& = SeekBuf&(bh%, 0, SBM_LineStart) 'and then ahead to the start of it
            DeleteBufLine bh% 'cut out the broken file link
            allOk% = 0 'delete OK status
        END IF
    WEND
    IF allOk% THEN
        result = idemessagebox("Remove Broken Links", "All files in the list are accessible.", "#OK")
    ELSE
        UpdateHistory "RECENT"
    END IF
END SUB

'Load the search history into the specified array used by the IDE.
'---------------------------------------------------------------------
SUB RetrieveSearchHistory (SearchHistory() AS STRING)
    bh% = OpenBuffer%("I", SearchedFile$) 'load and/or set pos (back) to start
    IF GetBufLen&(bh%) THEN
        REDIM SearchHistory(1 TO 100) AS STRING: lc% = 0
        WHILE EndOfBuf%(bh%) = 0 AND lc% < 100
           lc% = lc% + 1: SearchHistory(lc%) = ReadBufLine$(bh%)
        WEND
        REDIM _PRESERVE SearchHistory(1 TO lc%) AS STRING
    ELSE
       REDIM SearchHistory(1 TO 1) AS STRING
       SearchHistory(1) = ""
    END IF
END SUB

