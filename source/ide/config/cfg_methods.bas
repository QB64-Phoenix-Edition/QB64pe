'===== Routine to copy settings from another QB64-PE installation =============
SUB CopyFromOther
    cfoAgain:
    oqbi$ = _SELECTFOLDERDIALOG$("Select another QB64-PE installation...")
    IF oqbi$ <> "" THEN
        chkname$ = oqbi$ + pathsep$ + "qb64pe"
        IF _FILEEXISTS(chkname$) OR _FILEEXISTS(chkname$ + ".exe") THEN
            IF _DIREXISTS(oqbi$ + pathsep$ + ConfigFolder$) THEN
                oqbv% = 0 'since v3.14.0
                oqbi$ = oqbi$ + pathsep$ + ConfigFolder$ + pathsep$
                nul& = CopyFile&(oqbi$ + "config.ini", ConfigFile$)
            ELSE
                oqbv% = 1 'pre v3.14.0
                oqbi$ = oqbi$ + pathsep$ + "internal" + pathsep$
                nul& = CopyFile&(oqbi$ + "config.ini", ConfigFile$)
                IF nul& = 0 THEN 'we need to convert old to new color scheme order
                    oqbd$ = _READFILE$(ConfigFile$)
                    sid% = INSTR(oqbd$, "SchemeID=")
                    IF sid% > 0 THEN
                        id% = VAL(MID$(oqbd$, sid% + 9))
                        IF id% > 10 THEN
                            'custom user schemes move by 4, as of 4 new built-in schemes in v3.14.0
                            oqbd$ = LEFT$(oqbd$, sid% + 8) + str2$(id% + 4) + MID$(oqbd$, sid% + 9 + LEN(str2$(id%)))
                        ELSE
                            'built-in schemes are reordered according to the lookup string
                            ncso$ = "12349567de" 'old id = pick position for new id
                            oqbd$ = LEFT$(oqbd$, sid% + 8) + str2$(VAL("&H" + MID$(ncso$, id%, 1))) + MID$(oqbd$, sid% + 9 + LEN(str2$(id%)))
                        END IF
                    END IF
                    _WRITEFILE ConfigFile$, oqbd$
                END IF
                oqbi$ = oqbi$ + "temp" + pathsep$
            END IF
            nul& = CopyFile&(oqbi$ + "debug.ini", DebugFile$)
            IF nul& = 0 AND oqbv% = 1 THEN 'we need to convert, if pre v3.14.0
                oqbd$ = _READFILE$(DebugFile$)
                oqbd$ = StrReplace$(oqbd$, "[settings]", "[VWATCH PANEL 1]") 'new instance section name
                _WRITEFILE DebugFile$, oqbd$
            END IF
            nul& = CopyFile&(oqbi$ + "bookmarks.bin", BookmarksFile$)
            nul& = CopyFile&(oqbi$ + "recent.bin", RecentFile$)
            IF nul& = 0 AND oqbv% = 1 THEN 'we need to convert, if pre v3.14.0
                oqbd$ = _READFILE$(RecentFile$)
                oqbd$ = MID$(StrReplace$(oqbd$, CRLF + CRLF, CRLF), 3) 'remove empty lines
                oqbd$ = StrReplace$(oqbd$, CRLF, NATIVE_LINEENDING) 'make line endings native
                _WRITEFILE RecentFile$, oqbd$
            END IF
            nul& = CopyFile&(oqbi$ + "searched.bin", SearchedFile$)
            IF nul& = 0 AND oqbv% = 1 THEN 'we need to convert, if pre v3.14.0
                oqbd$ = _READFILE$(SearchedFile$)
                oqbd$ = StrReplace$(oqbd$, CRLF, NATIVE_LINEENDING) 'make line endings native
                _WRITEFILE SearchedFile$, oqbd$
            END IF
            'copying autosave & undo file(s) makes not much sense
        ELSE
            IF _MESSAGEBOX("QB64-PE IDE", "No qb64pe executable found, so that seems not to be a QB64-PE installation, select another folder?", "yesno", "warning") = 1 GOTO cfoAgain
        END IF
    END IF
    QB64_uptime# = TIMER(0.001) 'reinit to avoid startup resize events going wild
END SUB

'===== Routine to read/set initial config values ==============================
SUB ReadInitialConfig
    '--- General settings
    result = ReadConfigSetting(generalSettingsSection$, "DisableSyntaxHighlighter", value$)
    IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
        DisableSyntaxHighlighter = -1
        WriteConfigSetting generalSettingsSection$, "DisableSyntaxHighlighter", "True"
    ELSE
        DisableSyntaxHighlighter = 0
        WriteConfigSetting generalSettingsSection$, "DisableSyntaxHighlighter", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "PasteCursorAtEnd", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            PasteCursorAtEnd = -1
        ELSE
            PasteCursorAtEnd = 0
            WriteConfigSetting generalSettingsSection$, "PasteCursorAtEnd", "False"
        END IF
    ELSE
        PasteCursorAtEnd = -1
        WriteConfigSetting generalSettingsSection$, "PasteCursorAtEnd", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            ExeToSourceFolderFirstTimeMsg = -1
        ELSE
            ExeToSourceFolderFirstTimeMsg = 0
            WriteConfigSetting generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", "False"
        END IF
    ELSE
        ExeToSourceFolderFirstTimeMsg = 0
        WriteConfigSetting generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "WhiteListQB64FirstTimeMsg", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            WhiteListQB64FirstTimeMsg = -1
        ELSE
            WhiteListQB64FirstTimeMsg = 0
            WriteConfigSetting generalSettingsSection$, "WhiteListQB64FirstTimeMsg", "False"
        END IF
    ELSE
        WhiteListQB64FirstTimeMsg = 0
        WriteConfigSetting generalSettingsSection$, "WhiteListQB64FirstTimeMsg", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "SaveExeWithSource", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            SaveExeWithSource = -1
        ELSE
            SaveExeWithSource = 0
            WriteConfigSetting generalSettingsSection$, "SaveExeWithSource", "False"
        END IF
    ELSE
        SaveExeWithSource = 0
        WriteConfigSetting generalSettingsSection$, "SaveExeWithSource", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "EnableQuickNav", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            EnableQuickNav = -1
        ELSE
            EnableQuickNav = 0
            WriteConfigSetting generalSettingsSection$, "EnableQuickNav", "False"
        END IF
    ELSE
        EnableQuickNav = -1
        WriteConfigSetting generalSettingsSection$, "EnableQuickNav", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowErrorsImmediately", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            IDEShowErrorsImmediately = -1
        ELSE
            IDEShowErrorsImmediately = 0
            WriteConfigSetting generalSettingsSection$, "ShowErrorsImmediately", "False"
        END IF
    ELSE
        IDEShowErrorsImmediately = -1
        WriteConfigSetting generalSettingsSection$, "ShowErrorsImmediately", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowLineNumbers", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            ShowLineNumbers = -1
        ELSE
            ShowLineNumbers = 0
            WriteConfigSetting generalSettingsSection$, "ShowLineNumbers", "False"
        END IF
    ELSE
        ShowLineNumbers = -1
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbers", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowLineNumbersSeparator", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            ShowLineNumbersSeparator = -1
        ELSE
            ShowLineNumbersSeparator = 0
            WriteConfigSetting generalSettingsSection$, "ShowLineNumbersSeparator", "False"
        END IF
    ELSE
        ShowLineNumbersSeparator = -1
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbersSeparator", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowLineNumbersUseBG", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            ShowLineNumbersUseBG = -1
        ELSE
            ShowLineNumbersUseBG = 0
            WriteConfigSetting generalSettingsSection$, "ShowLineNumbersUseBG", "False"
        END IF
    ELSE
        ShowLineNumbersUseBG = -1
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbersUseBG", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "BracketHighlight", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            brackethighlight = -1
        ELSE
            brackethighlight = 0
            WriteConfigSetting generalSettingsSection$, "BracketHighlight", "False"
        END IF
    ELSE
        brackethighlight = -1
        WriteConfigSetting generalSettingsSection$, "BracketHighlight", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "KeywordHighlight", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            keywordHighlight = -1
        ELSE
            keywordHighlight = 0
            WriteConfigSetting generalSettingsSection$, "KeywordHighlight", "False"
        END IF
    ELSE
        keywordHighlight = -1
        WriteConfigSetting generalSettingsSection$, "KeywordHighlight", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "MultiHighlight", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            multihighlight = -1
        ELSE
            multihighlight = 0
            WriteConfigSetting generalSettingsSection$, "MultiHighlight", "False"
        END IF
    ELSE
        multihighlight = -1
        WriteConfigSetting generalSettingsSection$, "MultiHighlight", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "IgnoreWarnings", value$) THEN
        IF UCASE$(value$) = "TRUE" OR ABS(VAL(value$)) = 1 THEN
            IgnoreWarnings = -1
        ELSE
            IgnoreWarnings = 0
            WriteConfigSetting generalSettingsSection$, "IgnoreWarnings", "False"
        END IF
    ELSE
        IgnoreWarnings = 0
        WriteConfigSetting generalSettingsSection$, "IgnoreWarnings", "False"
    END IF

    result = ReadConfigSetting(generalSettingsSection$, "BackupSize", value$)
    idebackupsize = VAL(value$)
    IF idebackupsize < 10 OR idebackupsize > 2000 THEN
        idebackupsize = 500: WriteConfigSetting generalSettingsSection$, "BackupSize", "500"
    END IF

    result = ReadConfigSetting(generalSettingsSection$, "MaxRecentFiles", value$)
    ideMaxRecent = VAL(value$)
    IF ideMaxRecent < 5 OR ideMaxRecent > 200 THEN
        ideMaxRecent = 20: WriteConfigSetting generalSettingsSection$, "MaxRecentFiles", "20"
    END IF

    result = ReadConfigSetting(generalSettingsSection$, "MaxSearchStrings", value$)
    ideMaxSearch = VAL(value$)
    IF ideMaxSearch < 5 OR ideMaxSearch > 200 THEN
        ideMaxSearch = 50: WriteConfigSetting generalSettingsSection$, "MaxSearchStrings", "50"
    END IF

    result = ReadConfigSetting(generalSettingsSection$, "DebugInfo", value$)
    idedebuginfo = VAL(value$)
    IF UCASE$(LEFT$(value$, 4)) = "TRUE" THEN idedebuginfo = -1
    IF result = 0 OR idedebuginfo <> -1 THEN
        WriteConfigSetting generalSettingsSection$, "DebugInfo", "False"
        idedebuginfo = 0
    END IF
    Include_GDB_Debugging_Info = idedebuginfo

    wikiBaseAddress$ = "https://qb64phoenix.com/qb64wiki"
    IF ReadConfigSetting(generalSettingsSection$, "WikiBaseAddress", value$) THEN
        wikiBaseAddress$ = value$
    ELSE
        WriteConfigSetting generalSettingsSection$, "WikiBaseAddress", wikiBaseAddress$
    END IF

    UseGuiDialogs = ReadWriteBooleanSettingValue%(generalSettingsSection$, "UseGuiDialogs", -1)

    DefaultTerminal = ReadWriteStringSettingValue$(generalSettingsSection$, "DefaultTerminal", "")
    IF DefaultTerminal = "" AND os$ = "LNX" AND MacOSX = 0 THEN
        DefaultTerminal = findWorkingTerminal$
        WriteConfigSetting generalSettingsSection$, "DefaultTerminal", DefaultTerminal
    END IF

    LoggingEnabled = ReadWriteBooleanSettingValue%(generalSettingsSection$, "LoggingEnabled", 0)

    '--- Mouse settings
    result = ReadConfigSetting(mouseSettingsSection$, "SwapMouseButton", value$)
    IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
        MouseButtonSwapped = -1
        WriteConfigSetting mouseSettingsSection$, "SwapMouseButton", "True"
    ELSE
        MouseButtonSwapped = 0
        WriteConfigSetting mouseSettingsSection$, "SwapMouseButton", "False"
    END IF

    '--- Debug settings
    result = ReadConfigSetting(debugSettingsSection$, "BaseTCPPort", value$)
    idebaseTcpPort = VAL(value$)
    IF idebaseTcpPort = 0 THEN idebaseTcpPort = 9000: WriteConfigSetting debugSettingsSection$, "BaseTCPPort", "9000"

    result = ReadConfigSetting(debugSettingsSection$, "WatchListToConsole", value$)
    IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
        WatchListToConsole = -1
        WriteConfigSetting debugSettingsSection$, "WatchListToConsole", "True"
    ELSE
        WatchListToConsole = 0
        WriteConfigSetting debugSettingsSection$, "WatchListToConsole", "False"
    END IF

    IF ReadConfigSetting(debugSettingsSection$, "AutoAddDebugCommand", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            AutoAddDebugCommand = -1
        ELSE
            AutoAddDebugCommand = 0
            WriteConfigSetting debugSettingsSection$, "AutoAddDebugCommand", "False"
        END IF
    ELSE
        AutoAddDebugCommand = -1
        WriteConfigSetting debugSettingsSection$, "AutoAddDebugCommand", "True"
    END IF

    '--- Display settings
    IF ReadConfigSetting(displaySettingsSection$, "IDE_SortSUBs", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            idesortsubs = -1
        ELSE
            idesortsubs = 0
            WriteConfigSetting displaySettingsSection$, "IDE_SortSUBs", "False"
        END IF
    ELSE
        idesortsubs = 0
        WriteConfigSetting displaySettingsSection$, "IDE_SortSUBs", "False"
    END IF

    tmpKwCap = _FALSE: tmpKwLow = _FALSE
    IF ReadConfigSetting(displaySettingsSection$, "IDE_KeywordCapital", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN tmpKwCap = _TRUE ELSE tmpKwCap = _FALSE
    END IF
    IF ReadConfigSetting(displaySettingsSection$, "IDE_KeywordLowercase", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN tmpKwLow = _TRUE ELSE tmpKwLow = _FALSE
    END IF
    IF tmpKwCap = tmpKwLow THEN 'both set or unset = CaMeL case
        IDEAutoLayoutKwStyle = 0
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordCapital", "False"
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordLowercase", "False"
    ELSEIF tmpKwCap = _TRUE THEN '= UPPER case
        IDEAutoLayoutKwStyle = 1
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordLowercase", "False"
    ELSEIF tmpKwLow = _TRUE THEN '= lower case
        IDEAutoLayoutKwStyle = -1
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordCapital", "False"
    END IF

    IF ReadConfigSetting(displaySettingsSection$, "IDE_SUBsLength", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) = -1 THEN
            IDESubsLength = -1
        ELSE
            IDESubsLength = 0
            WriteConfigSetting displaySettingsSection$, "IDE_SUBsLength", "False"
        END IF
    ELSE
        IDESubsLength = -1
        WriteConfigSetting displaySettingsSection$, "IDE_SUBsLength", "True"
    END IF

    IF ReadConfigSetting(displaySettingsSection$, "IDE_AutoPosition", value$) THEN
        IF UCASE$(value$) = "TRUE" OR ABS(VAL(value$)) = 1 THEN
            IDE_AutoPosition = -1
        ELSE
            IDE_AutoPosition = 0
            WriteConfigSetting displaySettingsSection$, "IDE_AutoPosition", "False"
        END IF
    ELSE
        IDE_AutoPosition = -1
        WriteConfigSetting displaySettingsSection$, "IDE_AutoPosition", "True"
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_NormalCursorStart", value$)
    IDENormalCursorStart = VAL(value$)
    IF IDENormalCursorStart < 0 OR IDENormalCursorStart > 31 OR result = 0 THEN
        IDENormalCursorStart = 6
        WriteConfigSetting displaySettingsSection$, "IDE_NormalCursorStart", "6"
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_NormalCursorEnd", value$)
    IDENormalCursorEnd = VAL(value$)
    IF IDENormalCursorEnd < 0 OR IDENormalCursorEnd > 31 OR result = 0 THEN
        IDENormalCursorEnd = 8
        WriteConfigSetting displaySettingsSection$, "IDE_NormalCursorEnd", "8"
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_AutoFormat", value$)
    IDEAutoLayout = VAL(value$)
    IF UCASE$(value$) = "TRUE" OR IDEAutoLayout <> 0 THEN
        IDEAutoLayout = _TRUE
    ELSE
        IF UCASE$(value$) <> "FALSE" AND value$ <> "0" THEN
            WriteConfigSetting displaySettingsSection$, "IDE_AutoFormat", "True"
            IDEAutoLayout = _TRUE
        ELSE
            IDEAutoLayout = _FALSE
        END IF
    END IF
    DEFAutoLayout = IDEAutoLayout 'for restoring after '$FORMAT:OFF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_AutoIndent", value$)
    IDEAutoIndent = VAL(value$)
    IF UCASE$(value$) = "TRUE" OR IDEAutoIndent <> 0 THEN
        IDEAutoIndent = _TRUE
    ELSE
        IF UCASE$(value$) <> "FALSE" AND value$ <> "0" THEN
            WriteConfigSetting displaySettingsSection$, "IDE_AutoIndent", "True"
            IDEAutoIndent = _TRUE
        ELSE
            IDEAutoIndent = _FALSE
        END IF
    END IF
    DEFAutoIndent = IDEAutoIndent 'for restoring after '$FORMAT:OFF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_IndentSUBs", value$)
    IDEIndentSubs = VAL(value$)
    IF UCASE$(value$) = "TRUE" OR IDEIndentSubs <> 0 THEN
        IDEIndentSubs = _TRUE
    ELSE
        IF UCASE$(value$) <> "FALSE" AND value$ <> "0" THEN
            WriteConfigSetting displaySettingsSection$, "IDE_IndentSUBs", "True"
            IDEIndentSubs = _TRUE
        ELSE
            IDEIndentSubs = _FALSE
        END IF
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_IndentSize", value$)
    IDEAutoIndentSize = VAL(value$)
    IF IDEAutoIndentSize < 1 OR IDEAutoIndentSize > 64 THEN
        IDEAutoIndentSize = 4
        WriteConfigSetting displaySettingsSection$, "IDE_IndentSize", "4"
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFont", value$)
    idecustomfont = VAL(value$)
    IF UCASE$(value$) = "TRUE" OR idecustomfont <> 0 THEN
        idecustomfont = -1
    ELSE
        WriteConfigSetting displaySettingsSection$, "IDE_CustomFont", "False"
        idecustomfont = 0
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_UseFont8", value$)
    IF UCASE$(value$) = "TRUE" THEN
        IDE_UseFont8 = -1
    ELSE
        WriteConfigSetting displaySettingsSection$, "IDE_UseFont8", "False"
        IDE_UseFont8 = 0
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFont$", value$)
    idecustomfontfile$ = value$
    IF result = 0 OR idecustomfontfile$ = "" THEN
        IF os$ = "LNX" THEN
            idecustomfontfile$ = _DIR$("fonts") + "truetype/liberation/LiberationMono-Regular.ttf"
            IF MacOSX THEN idecustomfontfile$ = _DIR$("fonts") + "Courier New.ttf"
        ELSE
            idecustomfontfile$ = _DIR$("fonts") + "lucon.ttf"
        END IF
        WriteConfigSetting displaySettingsSection$, "IDE_CustomFont$", idecustomfontfile$
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFontSize", value$)
    idecustomfontheight = VAL(value$)
    IF idecustomfontheight < 8 OR idecustomfontheight > 100 THEN
        idecustomfontheight = 19
        WriteConfigSetting displaySettingsSection$, "IDE_CustomFontSize", STR$(idecustomfontheight)
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_CodePage", value$)
    idecpindex = VAL(value$)
    IF idecpindex < 0 OR idecpindex > idecpnum THEN idecpindex = 0: WriteConfigSetting displaySettingsSection$, "IDE_CodePage", "0"

    '--- Custom keywords
    IF ReadConfigSetting(customDictionarySection$, "CustomKeywords$", value$) THEN
        tempList$ = ""
        listOfCustomKeywords$ = "@" + UCASE$(value$) + "@"
        FOR I = 1 TO LEN(listOfCustomKeywords$)
            checkChar = ASC(listOfCustomKeywords$, I)
            IF checkChar = 64 THEN
                IF RIGHT$(tempList$, 1) <> "@" THEN tempList$ = tempList$ + "@"
            ELSE
                tempList$ = tempList$ + CHR$(checkChar)
            END IF
        NEXT
        listOfCustomKeywords$ = tempList$
        customKeywordsLength = LEN(listOfCustomKeywords$)
    ELSE
        IniSetAddQuotes -1
        WriteConfigSetting customDictionarySection$, "Instructions1", "Add custom keywords separated by the 'at' sign."
        WriteConfigSetting customDictionarySection$, "Instructions2", "Useful to colorize constants (eg @true@false@)."
        IniSetAddQuotes 0
        WriteConfigSetting customDictionarySection$, "CustomKeywords$", "@"
    END IF

    '--- Color schemes
    IniSetAddQuotes -1
    WriteConfigSetting colorSchemesSection$, "Instructions1", "Create custom color schemes in the IDE (Options->IDE Colors)."
    WriteConfigSetting colorSchemesSection$, "Instructions2", "Custom color schemes will be stored in this section."
    IniSetAddQuotes 0

    '--- Individual window settings (per instance)
    IF ReadConfigSetting(windowSettingsSection$, "IDE_TopPosition", value$) THEN
        IDE_TopPosition = VAL(value$)
        IDE_BypassAutoPosition = 0 'reset bypass, if positions become available
    ELSE
        IDE_BypassAutoPosition = -1 'if there's no position, then we don't need to auto-position
        IDE_TopPosition = 0
    END IF

    IF ReadConfigSetting(windowSettingsSection$, "IDE_LeftPosition", value$) THEN
        IDE_LeftPosition = VAL(value$)
        IDE_BypassAutoPosition = 0 'reset bypass, if positions become available
    ELSE
        IDE_BypassAutoPosition = -1 'if there's no position, then we don't need to auto-position
        IDE_LeftPosition = 0
    END IF

    result = ReadConfigSetting(windowSettingsSection$, "IDE_Width", value$)
    idewx = VAL(value$)
    IF idewx < 80 OR idewx > 999 THEN idewx = 120: WriteConfigSetting windowSettingsSection$, "IDE_Width", "120"

    result = ReadConfigSetting(windowSettingsSection$, "IDE_Height", value$)
    idewy = VAL(value$)
    IF idewy < 25 OR idewy > 999 THEN idewy = 40: WriteConfigSetting windowSettingsSection$, "IDE_Height", "40"

    '--- Color settings (default = Super Dark Blue scheme, as of v1.5)
    IDETextColor = _RGB32(216, 216, 216)
    IDEKeywordColor = _RGB32(69, 118, 147)
    IDENumbersColor = _RGB32(216, 98, 78)
    IDEErrorColor = _RGB32(170, 0, 0)
    IDEQuoteColor = _RGB32(255, 167, 0)
    IDEMetaCommandColor = _RGB32(85, 206, 85)
    IDECommentColor = _RGB32(98, 98, 98)
    IDEChromaColor = _RGB32(170, 170, 170)
    IDEBackgroundColor = _RGB32(0, 0, 39)
    IDEBackgroundColor2 = _RGB32(0, 49, 78)
    IDEBracketHighlightColor = _RGB32(0, 88, 108)

    '--- Manual/unsaved color settings
    IF ReadConfigSetting(colorSettingsSection$, "SchemeID", value$) = 0 THEN
        WriteConfigSetting colorSettingsSection$, "SchemeID", "1"
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "TextColor", value$) THEN
        IDETextColor = VRGBS(value$, IDETextColor)
    ELSE WriteConfigSetting colorSettingsSection$, "TextColor", rgbs$(IDETextColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "KeywordColor", value$) THEN
        IDEKeywordColor = VRGBS(value$, IDEKeywordColor)
    ELSE WriteConfigSetting colorSettingsSection$, "KeywordColor", rgbs$(IDEKeywordColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "ErrorColor", value$) THEN
        IDEErrorColor = VRGBS(value$, IDENumbersColor)
    ELSE WriteConfigSetting colorSettingsSection$, "ErrorColor", rgbs$(IDEErrorColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "NumbersColor", value$) THEN
        IDENumbersColor = VRGBS(value$, IDENumbersColor)
    ELSE WriteConfigSetting colorSettingsSection$, "NumbersColor", rgbs$(IDENumbersColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "QuoteColor", value$) THEN
        IDEQuoteColor = VRGBS(value$, IDEQuoteColor)
    ELSE WriteConfigSetting colorSettingsSection$, "QuoteColor", rgbs$(IDEQuoteColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "CommentColor", value$) THEN
        IDECommentColor = VRGBS(value$, IDECommentColor)
    ELSE WriteConfigSetting colorSettingsSection$, "CommentColor", rgbs$(IDECommentColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "ChromaColor", value$) THEN
        IDEChromaColor = VRGBS(value$, IDEChromaColor)
    ELSE WriteConfigSetting colorSettingsSection$, "ChromaColor", rgbs$(IDEChromaColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "MetaCommandColor", value$) THEN
        IDEMetaCommandColor = VRGBS(value$, IDEMetaCommandColor)
    ELSE WriteConfigSetting colorSettingsSection$, "MetaCommandColor", rgbs$(IDEMetaCommandColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "HighlightColor", value$) THEN
        IDEBracketHighlightColor = VRGBS(value$, IDEBracketHighlightColor)
    ELSE WriteConfigSetting colorSettingsSection$, "HighlightColor", rgbs$(IDEBracketHighlightColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "BackgroundColor", value$) THEN
        IDEBackgroundColor = VRGBS(value$, IDEBackgroundColor)
    ELSE WriteConfigSetting colorSettingsSection$, "BackgroundColor", rgbs$(IDEBackgroundColor)
    END IF

    IF ReadConfigSetting(colorSettingsSection$, "BackgroundColor2", value$) THEN
        IDEBackgroundColor2 = VRGBS(value$, IDEBackgroundColor2)
    ELSE WriteConfigSetting colorSettingsSection$, "BackgroundColor2", rgbs$(IDEBackgroundColor2)
    END IF

    '--- Compiler Settings
    OptimizeCppProgram = ReadWriteBooleanSettingValue%(compilerSettingsSection$, "OptimizeCppProgram", 0)
    StripDebugSymbols = ReadWriteBooleanSettingValue%(compilerSettingsSection$, "StripDebugSymbols", -1)

    MaxParallelProcesses = ReadWriteLongSettingValue&(compilerSettingsSection$, "MaxParallelProcesses", 3)
    IF MaxParallelProcesses < 1 OR MaxParallelProcesses > 128 THEN
        MaxParallelProcesses = 3
        WriteConfigSetting compilerSettingsSection$, "MaxParallelProcesses", "3"
    END IF

    ExtraCppFlags = ReadWriteStringSettingValue$(compilerSettingsSection$, "ExtraCppFlags", "")
    ExtraLinkerFlags = ReadWriteStringSettingValue$(compilerSettingsSection$, "ExtraLinkerFlags", "")

    GenerateLicenseFile = ReadWriteBooleanSettingValue%(compilerSettingsSection$, "GenerateLicenseFile", 0)
END SUB

