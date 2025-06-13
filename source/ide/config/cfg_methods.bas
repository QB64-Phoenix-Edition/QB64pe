'===== Routine to copy settings from another QB64-PE installation =============
SUB CopyFromOther
    cfoAgain:
    oqbi$ = _SELECTFOLDERDIALOG$("Select another QB64-PE installation...")
    IF oqbi$ <> "" THEN
        chkname$ = oqbi$ + pathsep$ + "qb64pe"
        IF _FILEEXISTS(chkname$) OR _FILEEXISTS(chkname$ + ".exe") THEN
            IF _DIREXISTS(oqbi$ + pathsep$ + ConfigFolder$) THEN
                oqbv% = _FALSE 'since v3.14.0
                oqbi$ = oqbi$ + pathsep$ + ConfigFolder$ + pathsep$
                nul& = CopyFile&(oqbi$ + "config.ini", ConfigFile$)
            ELSE
                oqbv% = _TRUE 'pre v3.14.0
                oqbi$ = oqbi$ + pathsep$ + "internal" + pathsep$
                nul& = CopyFile&(oqbi$ + "config.ini", ConfigFile$)
                IF nul& = 0 THEN 'we need to convert old to new color scheme order
                    oqbd$ = _READFILE$(ConfigFile$)
                    sid% = INSTR(oqbd$, "SchemeID=")
                    IF sid% > 0 THEN
                        id% = VAL(MID$(oqbd$, sid% + 9))
                        IF id% > 10 THEN
                            'custom user schemes move by 4, as of 4 new built-in schemes in v3.14.0
                            oqbd$ = LEFT$(oqbd$, sid% + 8) + _TOSTR$(id% + 4) + MID$(oqbd$, sid% + 9 + LEN(_TOSTR$(id%)))
                        ELSE
                            'built-in schemes are reordered according to the lookup string
                            ncso$ = "12349567de" 'old id = pick position for new id
                            oqbd$ = LEFT$(oqbd$, sid% + 8) + _TOSTR$(VAL("&H" + MID$(ncso$, id%, 1))) + MID$(oqbd$, sid% + 9 + LEN(_TOSTR$(id%)))
                        END IF
                    END IF
                    _WRITEFILE ConfigFile$, oqbd$
                END IF
                oqbi$ = oqbi$ + "temp" + pathsep$
            END IF
            nul& = CopyFile&(oqbi$ + "debug.ini", DebugFile$)
            IF nul& = 0 _ANDALSO oqbv% THEN 'we need to convert, if pre v3.14.0
                oqbd$ = _READFILE$(DebugFile$)
                oqbd$ = StrReplace$(oqbd$, "[settings]", "[VWATCH PANEL 1]") 'new instance section name
                _WRITEFILE DebugFile$, oqbd$
            END IF
            nul& = CopyFile&(oqbi$ + "bookmarks.bin", BookmarksFile$)
            nul& = CopyFile&(oqbi$ + "recent.bin", RecentFile$)
            IF nul& = 0 _ANDALSO oqbv% THEN 'we need to convert, if pre v3.14.0
                oqbd$ = _READFILE$(RecentFile$)
                oqbd$ = MID$(StrReplace$(oqbd$, CRLF + CRLF, CRLF), 3) 'remove empty lines
                oqbd$ = StrReplace$(oqbd$, CRLF, NATIVE_LINEENDING) 'make line endings native
                _WRITEFILE RecentFile$, oqbd$
            END IF
            nul& = CopyFile&(oqbi$ + "searched.bin", SearchedFile$)
            IF nul& = 0 _ANDALSO oqbv% THEN 'we need to convert, if pre v3.14.0
                oqbd$ = _READFILE$(SearchedFile$)
                oqbd$ = StrReplace$(oqbd$, CRLF, NATIVE_LINEENDING) 'make line endings native
                _WRITEFILE SearchedFile$, oqbd$
            END IF
            'apply changes in v4.2.0
            nul& = ReadConfigSetting(generalSettingsSection$, "DebugInfo", temp$)
            IniDeleteKey ConfigFile$, generalSettingsSection$, "DebugInfo"
            IF TFStringToBool%(temp$) = -2 THEN temp$ = "False"
            WriteConfigSetting compilerSettingsSection$, "IncludeDebugInfo", temp$
            '-----
            IF os$ = "WIN" THEN IniDeleteKey ConfigFile$, generalSettingsSection$, "DefaultTerminal"
            '-----
            nul& = ReadConfigSetting(generalSettingsSection$, "LoggingEnabled", temp$)
            IniDeleteKey ConfigFile$, generalSettingsSection$, "LoggingEnabled"
            IF TFStringToBool%(temp$) <> -1 THEN
                WriteConfigSetting loggingSettingsSection$, "LogMinLevel", "None"
            ELSE
                WriteConfigSetting loggingSettingsSection$, "LogMinLevel", "Information"
                WriteConfigSetting loggingSettingsSection$, "LogScopes", "qb64,libqb,libqb-image,libqb-audio"
                WriteConfigSetting loggingSettingsSection$, "LogHandlers", "console"
            END IF
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
    IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
        DisableSyntaxHighlighter = _TRUE
        WriteConfigSetting generalSettingsSection$, "DisableSyntaxHighlighter", "True"
    ELSE
        DisableSyntaxHighlighter = _FALSE
        WriteConfigSetting generalSettingsSection$, "DisableSyntaxHighlighter", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "PasteCursorAtEnd", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            PasteCursorAtEnd = _TRUE
        ELSE
            PasteCursorAtEnd = _FALSE
            WriteConfigSetting generalSettingsSection$, "PasteCursorAtEnd", "False"
        END IF
    ELSE
        PasteCursorAtEnd = _TRUE
        WriteConfigSetting generalSettingsSection$, "PasteCursorAtEnd", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            ExeToSourceFolderFirstTimeMsg = _TRUE
        ELSE
            ExeToSourceFolderFirstTimeMsg = _FALSE
            WriteConfigSetting generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", "False"
        END IF
    ELSE
        ExeToSourceFolderFirstTimeMsg = _FALSE
        WriteConfigSetting generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "WhiteListQB64FirstTimeMsg", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            WhiteListQB64FirstTimeMsg = _TRUE
        ELSE
            WhiteListQB64FirstTimeMsg = _FALSE
            WriteConfigSetting generalSettingsSection$, "WhiteListQB64FirstTimeMsg", "False"
        END IF
    ELSE
        WhiteListQB64FirstTimeMsg = _FALSE
        WriteConfigSetting generalSettingsSection$, "WhiteListQB64FirstTimeMsg", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "SaveExeWithSource", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            SaveExeWithSource = _TRUE
        ELSE
            SaveExeWithSource = _FALSE
            WriteConfigSetting generalSettingsSection$, "SaveExeWithSource", "False"
        END IF
    ELSE
        SaveExeWithSource = _FALSE
        WriteConfigSetting generalSettingsSection$, "SaveExeWithSource", "False"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "EnableQuickNav", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            EnableQuickNav = _TRUE
        ELSE
            EnableQuickNav = _FALSE
            WriteConfigSetting generalSettingsSection$, "EnableQuickNav", "False"
        END IF
    ELSE
        EnableQuickNav = _TRUE
        WriteConfigSetting generalSettingsSection$, "EnableQuickNav", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowErrorsImmediately", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            IDEShowErrorsImmediately = _TRUE
        ELSE
            IDEShowErrorsImmediately = _FALSE
            WriteConfigSetting generalSettingsSection$, "ShowErrorsImmediately", "False"
        END IF
    ELSE
        IDEShowErrorsImmediately = _TRUE
        WriteConfigSetting generalSettingsSection$, "ShowErrorsImmediately", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowLineNumbers", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            ShowLineNumbers = _TRUE
        ELSE
            ShowLineNumbers = _FALSE
            WriteConfigSetting generalSettingsSection$, "ShowLineNumbers", "False"
        END IF
    ELSE
        ShowLineNumbers = _TRUE
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbers", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowLineNumbersSeparator", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            ShowLineNumbersSeparator = _TRUE
        ELSE
            ShowLineNumbersSeparator = _FALSE
            WriteConfigSetting generalSettingsSection$, "ShowLineNumbersSeparator", "False"
        END IF
    ELSE
        ShowLineNumbersSeparator = _TRUE
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbersSeparator", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "ShowLineNumbersUseBG", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            ShowLineNumbersUseBG = _TRUE
        ELSE
            ShowLineNumbersUseBG = _FALSE
            WriteConfigSetting generalSettingsSection$, "ShowLineNumbersUseBG", "False"
        END IF
    ELSE
        ShowLineNumbersUseBG = _TRUE
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbersUseBG", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "BracketHighlight", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            BracketHighlight = _TRUE
        ELSE
            BracketHighlight = _FALSE
            WriteConfigSetting generalSettingsSection$, "BracketHighlight", "False"
        END IF
    ELSE
        BracketHighlight = _TRUE
        WriteConfigSetting generalSettingsSection$, "BracketHighlight", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "KeywordHighlight", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            KeywordHighlight = _TRUE
        ELSE
            KeywordHighlight = _FALSE
            WriteConfigSetting generalSettingsSection$, "KeywordHighlight", "False"
        END IF
    ELSE
        KeywordHighlight = _TRUE
        WriteConfigSetting generalSettingsSection$, "KeywordHighlight", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "MultiHighlight", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            MultiHighlight = _TRUE
        ELSE
            MultiHighlight = _FALSE
            WriteConfigSetting generalSettingsSection$, "MultiHighlight", "False"
        END IF
    ELSE
        MultiHighlight = _TRUE
        WriteConfigSetting generalSettingsSection$, "MultiHighlight", "True"
    END IF

    IF ReadConfigSetting(generalSettingsSection$, "IgnoreWarnings", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            IgnoreWarnings = _TRUE
        ELSE
            IgnoreWarnings = _FALSE
            WriteConfigSetting generalSettingsSection$, "IgnoreWarnings", "False"
        END IF
    ELSE
        IgnoreWarnings = _FALSE
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

    wikiBaseAddress$ = "https://qb64phoenix.com/qb64wiki"
    IF ReadConfigSetting(generalSettingsSection$, "WikiBaseAddress", value$) THEN
        wikiBaseAddress$ = value$
    ELSE
        WriteConfigSetting generalSettingsSection$, "WikiBaseAddress", wikiBaseAddress$
    END IF

    UseGuiDialogs = ReadWriteBooleanSettingValue%(generalSettingsSection$, "UseGuiDialogs", _TRUE)

    IF os$ = "LNX" AND MacOSX = 0 THEN
        DefaultTerminal$ = ReadWriteStringSettingValue$(generalSettingsSection$, "DefaultTerminal", "")
        IF DefaultTerminal$ = "" THEN
            DefaultTerminal$ = findWorkingTerminal$
            WriteConfigSetting generalSettingsSection$, "DefaultTerminal", DefaultTerminal$
        END IF
    END IF

    '--- Mouse settings
    result = ReadConfigSetting(mouseSettingsSection$, "SwapMouseButton", value$)
    IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
        MouseButtonSwapped = _TRUE
        WriteConfigSetting mouseSettingsSection$, "SwapMouseButton", "True"
    ELSE
        MouseButtonSwapped = _FALSE
        WriteConfigSetting mouseSettingsSection$, "SwapMouseButton", "False"
    END IF

    '--- Debug settings
    result = ReadConfigSetting(debugSettingsSection$, "BaseTCPPort", value$)
    idebaseTcpPort = VAL(value$)
    IF idebaseTcpPort = 0 THEN idebaseTcpPort = 9000: WriteConfigSetting debugSettingsSection$, "BaseTCPPort", "9000"

    result = ReadConfigSetting(debugSettingsSection$, "WatchListToConsole", value$)
    IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
        WatchListToConsole = _TRUE
        WriteConfigSetting debugSettingsSection$, "WatchListToConsole", "True"
    ELSE
        WatchListToConsole = _FALSE
        WriteConfigSetting debugSettingsSection$, "WatchListToConsole", "False"
    END IF

    IF ReadConfigSetting(debugSettingsSection$, "AutoAddDebugCommand", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            AutoAddDebugCommand = _TRUE
        ELSE
            AutoAddDebugCommand = _FALSE
            WriteConfigSetting debugSettingsSection$, "AutoAddDebugCommand", "False"
        END IF
    ELSE
        AutoAddDebugCommand = _TRUE
        WriteConfigSetting debugSettingsSection$, "AutoAddDebugCommand", "True"
    END IF

    '--- Logging settings
    LogMinLevel$ = ReadWriteStringSettingValue$(loggingSettingsSection$, "LogMinLevel", "None")
    LogScopes$ = ReadWriteStringSettingValue$(loggingSettingsSection$, "LogScopes", "qb64")
    LogHandlers$ = ReadWriteStringSettingValue$(loggingSettingsSection$, "LogHandlers", "console")
    LogFileName$ = ReadWriteStringSettingValue$(loggingSettingsSection$, "LogFileName", _STARTDIR$ + "qb64pe-log.txt")
    'update global enable states
    LoggingEnabled = (LogMinLevel$ <> "None")
    LogToConsole = LoggingEnabled AND (INSTR("," + LogHandlers$ + ",", ",console,") > 0)

    '--- Display settings
    IF ReadConfigSetting(displaySettingsSection$, "IDE_SortSUBs", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            IDESortSubs = _TRUE
        ELSE
            IDESortSubs = _FALSE
            WriteConfigSetting displaySettingsSection$, "IDE_SortSUBs", "False"
        END IF
    ELSE
        IDESortSubs = _FALSE
        WriteConfigSetting displaySettingsSection$, "IDE_SortSUBs", "False"
    END IF

    tmpKwCap = _FALSE: tmpKwLow = _FALSE
    IF ReadConfigSetting(displaySettingsSection$, "IDE_KeywordCapital", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN tmpKwCap = _TRUE ELSE tmpKwCap = _FALSE
    END IF
    IF ReadConfigSetting(displaySettingsSection$, "IDE_KeywordLowercase", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN tmpKwLow = _TRUE ELSE tmpKwLow = _FALSE
    END IF
    IF tmpKwCap = tmpKwLow THEN 'both set or unset = CaMeL case
        IDEAutoLayoutKwStyle = _EQUAL
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordCapital", "False"
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordLowercase", "False"
    ELSEIF tmpKwCap = _TRUE THEN '= UPPER case
        IDEAutoLayoutKwStyle = _GREATER
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordLowercase", "False"
    ELSEIF tmpKwLow = _TRUE THEN '= lower case
        IDEAutoLayoutKwStyle = _LESS
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordCapital", "False"
    END IF

    IF ReadConfigSetting(displaySettingsSection$, "IDE_SUBsLength", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            IDESubsLength = _TRUE
        ELSE
            IDESubsLength = _FALSE
            WriteConfigSetting displaySettingsSection$, "IDE_SUBsLength", "False"
        END IF
    ELSE
        IDESubsLength = _TRUE
        WriteConfigSetting displaySettingsSection$, "IDE_SUBsLength", "True"
    END IF

    IF ReadConfigSetting(displaySettingsSection$, "IDE_AutoPosition", value$) THEN
        IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
            IDEAutoPosition = _TRUE
        ELSE
            IDEAutoPosition = _FALSE
            WriteConfigSetting displaySettingsSection$, "IDE_AutoPosition", "False"
        END IF
    ELSE
        IDEAutoPosition = _TRUE
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
    IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
        IDECustomFont = _TRUE
    ELSE
        WriteConfigSetting displaySettingsSection$, "IDE_CustomFont", "False"
        IDECustomFont = _FALSE
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_UseFont8", value$)
    IF UCASE$(value$) = "TRUE" OR VAL(value$) <> 0 THEN
        IDEUseFont8 = _TRUE
    ELSE
        WriteConfigSetting displaySettingsSection$, "IDE_UseFont8", "False"
        IDEUseFont8 = _FALSE
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFont$", value$)
    IDECustomFontFile$ = value$
    IF result = 0 OR IDECustomFontFile$ = "" THEN
        IF os$ = "LNX" THEN
            IDECustomFontFile$ = _DIR$("fonts") + "truetype/liberation/LiberationMono-Regular.ttf"
            IF MacOSX THEN IDECustomFontFile$ = _DIR$("fonts") + "Courier New.ttf"
        ELSE
            IDECustomFontFile$ = _DIR$("fonts") + "lucon.ttf"
        END IF
        WriteConfigSetting displaySettingsSection$, "IDE_CustomFont$", IDECustomFontFile$
    END IF

    result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFontSize", value$)
    IDECustomFontHeight = VAL(value$)
    IF IDECustomFontHeight < 8 OR IDECustomFontHeight > 99 THEN
        IDECustomFontHeight = 19
        WriteConfigSetting displaySettingsSection$, "IDE_CustomFontSize", _TOSTR$(IDECustomFontHeight)
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
        IDETopPosition = VAL(value$)
        IDEBypassAutoPosition = _FALSE 'reset bypass, if positions become available
    ELSE
        IDEBypassAutoPosition = _TRUE 'if there's no position, then we don't need to auto-position
        IDETopPosition = 0
    END IF

    IF ReadConfigSetting(windowSettingsSection$, "IDE_LeftPosition", value$) THEN
        IDELeftPosition = VAL(value$)
        IDEBypassAutoPosition = _FALSE 'reset bypass, if positions become available
    ELSE
        IDEBypassAutoPosition = _TRUE 'if there's no position, then we don't need to auto-position
        IDELeftPosition = 0
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
    IncludeDebugInfo = ReadWriteBooleanSettingValue%(compilerSettingsSection$, "IncludeDebugInfo", 0)

    MaxParallelProcesses = ReadWriteLongSettingValue&(compilerSettingsSection$, "MaxParallelProcesses", 3)
    IF MaxParallelProcesses < 1 OR MaxParallelProcesses > 128 THEN
        MaxParallelProcesses = 3
        WriteConfigSetting compilerSettingsSection$, "MaxParallelProcesses", "3"
    END IF

    ExtraCppFlags$ = ReadWriteStringSettingValue$(compilerSettingsSection$, "ExtraCppFlags", "")
    ExtraLinkerFlags$ = ReadWriteStringSettingValue$(compilerSettingsSection$, "ExtraLinkerFlags", "")

    GenerateLicenseFile = ReadWriteBooleanSettingValue%(compilerSettingsSection$, "GenerateLicenseFile", 0)

    IF os$ = "WIN" THEN
        UseSystemMinGW = ReadWriteBooleanSettingValue%(compilerSettingsSection$, "UseSystemMinGW", _FALSE)
    ELSE
        UseSystemMinGW = _TRUE 'always use the system compiler on non-Windows platforms
    END IF
END SUB

