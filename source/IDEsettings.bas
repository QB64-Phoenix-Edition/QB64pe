'**********************************
'***
'***   Set DEBUG to TRUE to have syntax checking and autoformating work without any error
'***   Just remember to set DEBUG back to FALSE before saving and exiting
'***
'*********************************
$Let DEBUG = FALSE

Dim Shared IDECommentColor As _Unsigned Long, IDEMetaCommandColor As _Unsigned Long
Dim Shared IDEQuoteColor As _Unsigned Long, IDETextColor As _Unsigned Long
Dim Shared IDEBackgroundColor As _Unsigned Long, IDEChromaColor As _Unsigned Long
Dim Shared IDEBackgroundColor2 As _Unsigned Long, IDEBracketHighlightColor As _Unsigned Long
Dim Shared IDEKeywordColor As _Unsigned Long, IDENumbersColor As _Unsigned Long
Dim Shared IDE_AutoPosition As _Byte, IDE_TopPosition As Integer, IDE_LeftPosition As Integer
Dim Shared IDE_BypassAutoPosition As _Byte, idesortsubs As _Byte, IDESubsLength As _Byte
Dim Shared IDENormalCursorStart As Long, IDENormalCursorEnd As Long
Dim Shared MouseButtonSwapped As _Byte
Dim Shared PasteCursorAtEnd As _Byte
Dim Shared SaveExeWithSource As _Byte, EnableQuickNav As _Byte
Dim Shared IDEShowErrorsImmediately As _Byte
Dim Shared ShowLineNumbersSeparator As _Byte, ShowLineNumbersUseBG As _Byte
Dim Shared IgnoreWarnings As _Byte, qb64versionprinted As _Byte
Dim Shared DisableSyntaxHighlighter As _Byte, ExeToSourceFolderFirstTimeMsg As _Byte
Dim Shared WhiteListQB64FirstTimeMsg As _Byte, ideautolayoutkwcapitals As _Byte
Dim Shared WatchListToConsole As _Byte
Dim Shared windowSettingsSection$, colorSettingsSection$, customDictionarySection$
Dim Shared mouseSettingsSection$, generalSettingsSection$, displaySettingsSection$
Dim Shared colorSchemesSection$, debugSettingsSection$, iniFolderIndex$, DebugInfoIniWarning$, ConfigFile$
Dim Shared idebaseTcpPort As Long, AutoAddDebugCommand As _Byte
Dim Shared wikiBaseAddress$

ConfigFile$ = "internal/config.ini"
iniFolderIndex$ = Str$(tempfolderindex)
DebugInfoIniWarning$ = " 'Do not change manually. Use 'qb64 -s', or Debug->Advanced in the IDE"

windowSettingsSection$ = "IDE WINDOW" + iniFolderIndex$
colorSettingsSection$ = "IDE COLOR SETTINGS" + iniFolderIndex$
colorSchemesSection$ = "IDE COLOR SCHEMES"
customDictionarySection$ = "CUSTOM DICTIONARIES"
mouseSettingsSection$ = "MOUSE SETTINGS"
generalSettingsSection$ = "GENERAL SETTINGS"
displaySettingsSection$ = "IDE DISPLAY SETTINGS"
debugSettingsSection$ = "DEBUG SETTINGS"


IniDisableAddQuotes = -1
IniForceReload = -1
IniAllowBasicComments = -1
IniDisableAutoCommit = 0

'General settings -------------------------------------------------------------
result = ReadConfigSetting(generalSettingsSection$, "DisableSyntaxHighlighter", value$)
If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
    DisableSyntaxHighlighter = -1
    WriteConfigSetting generalSettingsSection$, "DisableSyntaxHighlighter", "True"
Else
    DisableSyntaxHighlighter = 0
    WriteConfigSetting generalSettingsSection$, "DisableSyntaxHighlighter", "False"
End If

If ReadConfigSetting(generalSettingsSection$, "PasteCursorAtEnd", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        PasteCursorAtEnd = -1
    Else
        PasteCursorAtEnd = 0
        WriteConfigSetting generalSettingsSection$, "PasteCursorAtEnd", "False"
    End If
Else
    PasteCursorAtEnd = -1
    WriteConfigSetting generalSettingsSection$, "PasteCursorAtEnd", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        ExeToSourceFolderFirstTimeMsg = -1
    Else
        ExeToSourceFolderFirstTimeMsg = 0
        WriteConfigSetting generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", "False"
    End If
Else
    ExeToSourceFolderFirstTimeMsg = 0
    WriteConfigSetting generalSettingsSection$, "ExeToSourceFolderFirstTimeMsg", "False"
End If

If ReadConfigSetting(generalSettingsSection$, "WhiteListQB64FirstTimeMsg", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        WhiteListQB64FirstTimeMsg = -1
    Else
        WhiteListQB64FirstTimeMsg = 0
        WriteConfigSetting generalSettingsSection$, "WhiteListQB64FirstTimeMsg", "False"
    End If
Else
    WhiteListQB64FirstTimeMsg = 0
    WriteConfigSetting generalSettingsSection$, "WhiteListQB64FirstTimeMsg", "False"
End If

If ReadConfigSetting(generalSettingsSection$, "SaveExeWithSource", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        SaveExeWithSource = -1
    Else
        SaveExeWithSource = 0
        WriteConfigSetting generalSettingsSection$, "SaveExeWithSource", "False"
    End If
Else
    SaveExeWithSource = 0
    WriteConfigSetting generalSettingsSection$, "SaveExeWithSource", "False"
End If

If ReadConfigSetting(generalSettingsSection$, "EnableQuickNav", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        EnableQuickNav = -1
    Else
        EnableQuickNav = 0
        WriteConfigSetting generalSettingsSection$, "EnableQuickNav", "False"
    End If
Else
    EnableQuickNav = -1
    WriteConfigSetting generalSettingsSection$, "EnableQuickNav", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "ShowErrorsImmediately", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        IDEShowErrorsImmediately = -1
    Else
        IDEShowErrorsImmediately = 0
        WriteConfigSetting generalSettingsSection$, "ShowErrorsImmediately", "False"
    End If
Else
    IDEShowErrorsImmediately = -1
    WriteConfigSetting generalSettingsSection$, "ShowErrorsImmediately", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "ShowLineNumbers", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        ShowLineNumbers = -1
    Else
        ShowLineNumbers = 0
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbers", "False"
    End If
Else
    ShowLineNumbers = -1
    WriteConfigSetting generalSettingsSection$, "ShowLineNumbers", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "ShowLineNumbersSeparator", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        ShowLineNumbersSeparator = -1
    Else
        ShowLineNumbersSeparator = 0
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbersSeparator", "False"
    End If
Else
    ShowLineNumbersSeparator = -1
    WriteConfigSetting generalSettingsSection$, "ShowLineNumbersSeparator", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "ShowLineNumbersUseBG", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        ShowLineNumbersUseBG = -1
    Else
        ShowLineNumbersUseBG = 0
        WriteConfigSetting generalSettingsSection$, "ShowLineNumbersUseBG", "False"
    End If
Else
    ShowLineNumbersUseBG = -1
    WriteConfigSetting generalSettingsSection$, "ShowLineNumbersUseBG", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "BracketHighlight", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        brackethighlight = -1
    Else
        brackethighlight = 0
        WriteConfigSetting generalSettingsSection$, "BracketHighlight", "False"
    End If
Else
    brackethighlight = -1
    WriteConfigSetting generalSettingsSection$, "BracketHighlight", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "KeywordHighlight", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        keywordHighlight = -1
    Else
        keywordHighlight = 0
        WriteConfigSetting generalSettingsSection$, "KeywordHighlight", "False"
    End If
Else
    keywordHighlight = -1
    WriteConfigSetting generalSettingsSection$, "KeywordHighlight", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "MultiHighlight", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        multihighlight = -1
    Else
        multihighlight = 0
        WriteConfigSetting generalSettingsSection$, "MultiHighlight", "False"
    End If
Else
    multihighlight = -1
    WriteConfigSetting generalSettingsSection$, "MultiHighlight", "True"
End If

If ReadConfigSetting(generalSettingsSection$, "IgnoreWarnings", value$) Then
    If UCase$(value$) = "TRUE" Or Abs(Val(value$)) = 1 Then
        IgnoreWarnings = -1
    Else
        IgnoreWarnings = 0
        WriteConfigSetting generalSettingsSection$, "IgnoreWarnings", "False"
    End If
Else
    IgnoreWarnings = 0
    WriteConfigSetting generalSettingsSection$, "IgnoreWarnings", "False"
End If

result = ReadConfigSetting(generalSettingsSection$, "BackupSize", value$)
idebackupsize = Val(value$)
If idebackupsize < 10 Or idebackupsize > 2000 Then idebackupsize = 100: WriteConfigSetting generalSettingsSection$, "BackupSize", "100 'in MB"

result = ReadConfigSetting(generalSettingsSection$, "DebugInfo", value$)
idedebuginfo = Val(value$)
If UCase$(Left$(value$, 4)) = "TRUE" Then idedebuginfo = 1
If result = 0 Or idedebuginfo <> 1 Then
    WriteConfigSetting generalSettingsSection$, "DebugInfo", "False" + DebugInfoIniWarning$
    idedebuginfo = 0
End If
Include_GDB_Debugging_Info = idedebuginfo

wikiBaseAddress$ = "https://qb64phoenix.com/qb64wiki"
If ReadConfigSetting(generalSettingsSection$, "WikiBaseAddress", value$) Then
    wikiBaseAddress$ = value$
Else WriteConfigSetting generalSettingsSection$, "WikiBaseAddress", wikiBaseAddress$
End If


'Mouse settings ---------------------------------------------------------------
result = ReadConfigSetting(mouseSettingsSection$, "SwapMouseButton", value$)
If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
    MouseButtonSwapped = -1
    WriteConfigSetting mouseSettingsSection$, "SwapMouseButton", "True"
Else
    MouseButtonSwapped = 0
    WriteConfigSetting mouseSettingsSection$, "SwapMouseButton", "False"
End If

'Debug settings ---------------------------------------------------------------
result = ReadConfigSetting(debugSettingsSection$, "BaseTCPPort", value$)
idebaseTcpPort = Val(value$)
If idebaseTcpPort = 0 Then idebaseTcpPort = 9000: WriteConfigSetting debugSettingsSection$, "BaseTCPPort", "9000"

result = ReadConfigSetting(debugSettingsSection$, "WatchListToConsole", value$)
If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
    WatchListToConsole = -1
    WriteConfigSetting debugSettingsSection$, "WatchListToConsole", "True"
Else
    WatchListToConsole = 0
    WriteConfigSetting debugSettingsSection$, "WatchListToConsole", "False"
End If

If ReadConfigSetting(debugSettingsSection$, "AutoAddDebugCommand", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        AutoAddDebugCommand = -1
    Else
        AutoAddDebugCommand = 0
        WriteConfigSetting debugSettingsSection$, "AutoAddDebugCommand", "False"
    End If
Else
    AutoAddDebugCommand = -1
    WriteConfigSetting debugSettingsSection$, "AutoAddDebugCommand", "True"
End If

'Display settings -------------------------------------------------------------
If ReadConfigSetting(displaySettingsSection$, "IDE_SortSUBs", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        idesortsubs = -1
    Else
        idesortsubs = 0
        WriteConfigSetting displaySettingsSection$, "IDE_SortSUBs", "False"
    End If
Else
    idesortsubs = 0
    WriteConfigSetting displaySettingsSection$, "IDE_SortSUBs", "False"
End If

If ReadConfigSetting(displaySettingsSection$, "IDE_KeywordCapital", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        ideautolayoutkwcapitals = -1
    Else
        ideautolayoutkwcapitals = 0
        WriteConfigSetting displaySettingsSection$, "IDE_KeywordCapital", "False"
    End If
Else
    ideautolayoutkwcapitals = 0
    WriteConfigSetting displaySettingsSection$, "IDE_KeywordCapital", "False"
End If

If ReadConfigSetting(displaySettingsSection$, "IDE_SUBsLength", value$) Then
    If UCase$(value$) = "TRUE" Or Val(value$) = -1 Then
        IDESubsLength = -1
    Else
        IDESubsLength = 0
        WriteConfigSetting displaySettingsSection$, "IDE_SUBsLength", "False"
    End If
Else
    IDESubsLength = -1
    WriteConfigSetting displaySettingsSection$, "IDE_SUBsLength", "True"
End If

If ReadConfigSetting(displaySettingsSection$, "IDE_AutoPosition", value$) Then
    If UCase$(value$) = "TRUE" Or Abs(Val(value$)) = 1 Then
        IDE_AutoPosition = -1
    Else
        IDE_AutoPosition = 0
        WriteConfigSetting displaySettingsSection$, "IDE_AutoPosition", "False"
    End If
Else
    IDE_AutoPosition = -1
    WriteConfigSetting displaySettingsSection$, "IDE_AutoPosition", "True"
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_NormalCursorStart", value$)
IDENormalCursorStart = Val(value$)
If IDENormalCursorStart < 0 Or IDENormalCursorStart > 31 Or result = 0 Then
    IDENormalCursorStart = 6
    WriteConfigSetting displaySettingsSection$, "IDE_NormalCursorStart", "6"
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_NormalCursorEnd", value$)
IDENormalCursorEnd = Val(value$)
If IDENormalCursorEnd < 0 Or IDENormalCursorEnd > 31 Or result = 0 Then
    IDENormalCursorEnd = 8
    WriteConfigSetting displaySettingsSection$, "IDE_NormalCursorEnd", "8"
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_AutoFormat", value$)
ideautolayout = Val(value$)
If UCase$(value$) = "TRUE" Or ideautolayout <> 0 Then
    ideautolayout = 1
Else
    If UCase$(value$) <> "FALSE" And value$ <> "0" Then
        WriteConfigSetting displaySettingsSection$, "IDE_AutoFormat", "True"
        ideautolayout = 1
    Else
        ideautolayout = 0
    End If
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_AutoIndent", value$)
ideautoindent = Val(value$)
If UCase$(value$) = "TRUE" Or ideautoindent <> 0 Then
    ideautoindent = 1
Else
    If UCase$(value$) <> "FALSE" And value$ <> "0" Then
        WriteConfigSetting displaySettingsSection$, "IDE_AutoIndent", "True"
        ideautoindent = 1
    Else
        ideautoindent = 0
    End If
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_IndentSUBs", value$)
ideindentsubs = Val(value$)
If UCase$(value$) = "TRUE" Or ideindentsubs <> 0 Then
    ideindentsubs = 1
Else
    If UCase$(value$) <> "FALSE" And value$ <> "0" Then
        WriteConfigSetting displaySettingsSection$, "IDE_IndentSUBs", "True"
        ideindentsubs = 1
    Else
        ideindentsubs = 0
    End If
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_IndentSize", value$)
ideautoindentsize = Val(value$)
If ideautoindentsize < 1 Or ideautoindentsize > 64 Then
    ideautoindentsize = 4
    WriteConfigSetting displaySettingsSection$, "IDE_IndentSize", "4"
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFont", value$)
idecustomfont = Val(value$)
If UCase$(value$) = "TRUE" Or idecustomfont <> 0 Then
    idecustomfont = 1
Else
    WriteConfigSetting displaySettingsSection$, "IDE_CustomFont", "False"
    idecustomfont = 0
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_UseFont8", value$)
If UCase$(value$) = "TRUE" Then
    IDE_UseFont8 = 1
Else
    WriteConfigSetting displaySettingsSection$, "IDE_UseFont8", "False"
    IDE_UseFont8 = 0
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFont$", value$)
idecustomfontfile$ = value$
If result = 0 Or idecustomfontfile$ = "" Then
    idecustomfontfile$ = "C:\Windows\Fonts\lucon.ttf"
    WriteConfigSetting displaySettingsSection$, "IDE_CustomFont$", idecustomfontfile$
End If

result = ReadConfigSetting(displaySettingsSection$, "IDE_CustomFontSize", value$)
idecustomfontheight = Val(value$)
If idecustomfontheight < 8 Or idecustomfontheight > 100 Then idecustomfontheight = 21: WriteConfigSetting displaySettingsSection$, "IDE_CustomFontSize", "21"

result = ReadConfigSetting(displaySettingsSection$, "IDE_CodePage", value$)
idecpindex = Val(value$)
If idecpindex < 0 Or idecpindex > idecpnum Then idecpindex = 0: WriteConfigSetting displaySettingsSection$, "IDE_CodePage", "0"

'Custom keywords --------------------------------------------------------------
If ReadConfigSetting(customDictionarySection$, "CustomKeywords$", value$) Then
    tempList$ = ""
    listOfCustomKeywords$ = "@" + UCase$(value$) + "@"
    For I = 1 To Len(listOfCustomKeywords$)
        checkChar = Asc(listOfCustomKeywords$, I)
        If checkChar = 64 Then
            If Right$(tempList$, 1) <> "@" Then tempList$ = tempList$ + "@"
        Else
            tempList$ = tempList$ + Chr$(checkChar)
        End If
    Next
    listOfCustomKeywords$ = tempList$
    customKeywordsLength = Len(listOfCustomKeywords$)
Else
    IniSetAddQuotes -1
    WriteConfigSetting customDictionarySection$, "Instructions1", "Add custom keywords separated by the 'at' sign."
    WriteConfigSetting customDictionarySection$, "Instructions2", "Useful to colorize constants (eg @true@false@)."
    IniSetAddQuotes 0
    WriteConfigSetting customDictionarySection$, "CustomKeywords$", "@"
End If

'Color schemes ---------------------------------------------------------------
IniSetAddQuotes -1
WriteConfigSetting colorSchemesSection$, "Instructions1", "Create custom color schemes in the IDE (Options->IDE Colors)."
WriteConfigSetting colorSchemesSection$, "Instructions2", "Custom color schemes will be stored in this section."
IniSetAddQuotes 0

'Individual window settings (different for each running instance) -------------
If ReadConfigSetting(windowSettingsSection$, "IDE_TopPosition", value$) Then
    IDE_TopPosition = Val(value$)
Else
    IDE_BypassAutoPosition = -1 'If there's no position saved in the file, then we certainly don't need to try and auto-position to our last setting.
    IDE_TopPosition = 0
End If

If ReadConfigSetting(windowSettingsSection$, "IDE_LeftPosition", value$) Then
    IDE_LeftPosition = Val(value$)
Else
    IDE_BypassAutoPosition = -1 'If there's no position saved in the file, then we certainly don't need to try and auto-position to our last setting.
    IDE_LeftPosition = 0
End If

result = ReadConfigSetting(windowSettingsSection$, "IDE_Width", value$)
idewx = Val(value$)
If idewx < 80 Or idewx > 1000 Then idewx = 80: WriteConfigSetting windowSettingsSection$, "IDE_Width", "80"

result = ReadConfigSetting(windowSettingsSection$, "IDE_Height", value$)
idewy = Val(value$)
If idewy < 25 Or idewy > 1000 Then idewy = 25: WriteConfigSetting windowSettingsSection$, "IDE_Height", "25"

'Color settings ---------------------------------------------------------------
'Defaults: (= Super Dark Blue scheme, as of v1.5)
IDETextColor = _RGB32(216, 216, 216)
IDEKeywordColor = _RGB32(69, 118, 147)
IDENumbersColor = _RGB32(216, 98, 78)
IDEQuoteColor = _RGB32(255, 167, 0)
IDEMetaCommandColor = _RGB32(85, 206, 85)
IDECommentColor = _RGB32(98, 98, 98)
IDEChromaColor = _RGB32(170, 170, 170)
IDEBackgroundColor = _RGB32(0, 0, 39)
IDEBackgroundColor2 = _RGB32(0, 49, 78)
IDEBracketHighlightColor = _RGB32(0, 88, 108)

'Manual/unsaved color settings:
If ReadConfigSetting(colorSettingsSection$, "SchemeID", value$) = 0 Then
    WriteConfigSetting colorSettingsSection$, "SchemeID", "1"
End If

If ReadConfigSetting(colorSettingsSection$, "TextColor", value$) Then
    IDETextColor = VRGBS(value$, IDETextColor)
Else WriteConfigSetting colorSettingsSection$, "TextColor", rgbs$(IDETextColor)
End If

If ReadConfigSetting(colorSettingsSection$, "KeywordColor", value$) Then
    IDEKeywordColor = VRGBS(value$, IDEKeywordColor)
Else WriteConfigSetting colorSettingsSection$, "KeywordColor", rgbs$(IDEKeywordColor)
End If

If ReadConfigSetting(colorSettingsSection$, "NumbersColor", value$) Then
    IDENumbersColor = VRGBS(value$, IDENumbersColor)
Else WriteConfigSetting colorSettingsSection$, "NumbersColor", rgbs$(IDENumbersColor)
End If

If ReadConfigSetting(colorSettingsSection$, "QuoteColor", value$) Then
    IDEQuoteColor = VRGBS(value$, IDEQuoteColor)
Else WriteConfigSetting colorSettingsSection$, "QuoteColor", rgbs$(IDEQuoteColor)
End If

If ReadConfigSetting(colorSettingsSection$, "CommentColor", value$) Then
    IDECommentColor = VRGBS(value$, IDECommentColor)
Else WriteConfigSetting colorSettingsSection$, "CommentColor", rgbs$(IDECommentColor)
End If

If ReadConfigSetting(colorSettingsSection$, "ChromaColor", value$) Then
    IDEChromaColor = VRGBS(value$, IDEChromaColor)
Else WriteConfigSetting colorSettingsSection$, "ChromaColor", rgbs$(IDEChromaColor)
End If

If ReadConfigSetting(colorSettingsSection$, "MetaCommandColor", value$) Then
    IDEMetaCommandColor = VRGBS(value$, IDEMetaCommandColor)
Else WriteConfigSetting colorSettingsSection$, "MetaCommandColor", rgbs$(IDEMetaCommandColor)
End If

If ReadConfigSetting(colorSettingsSection$, "HighlightColor", value$) Then
    IDEBracketHighlightColor = VRGBS(value$, IDEBracketHighlightColor)
Else WriteConfigSetting colorSettingsSection$, "HighlightColor", rgbs$(IDEBracketHighlightColor)
End If

If ReadConfigSetting(colorSettingsSection$, "BackgroundColor", value$) Then
    IDEBackgroundColor = VRGBS(value$, IDEBackgroundColor)
Else WriteConfigSetting colorSettingsSection$, "BackgroundColor", rgbs$(IDEBackgroundColor)
End If

If ReadConfigSetting(colorSettingsSection$, "BackgroundColor2", value$) Then
    IDEBackgroundColor2 = VRGBS(value$, IDEBackgroundColor2)
Else WriteConfigSetting colorSettingsSection$, "BackgroundColor2", rgbs$(IDEBackgroundColor2)
End If

'End of initial settings ------------------------------------------------------

$If DEBUG = TRUE Then
    Function ReadConfigSetting (section$, item$, value$)
        value$ = ReadSetting$(ConfigFile$, section$, item$)
        ReadConfigSetting = (Len(value$) > 0)
    End Function
    Sub WriteConfigSetting (section$, item$, value$)
        WriteSetting ConfigFile$, section$, item$, value$
    End Sub
    Function VRGBS~& (text$, DefaultColor As _Unsigned Long)
        'Value of RGB String = VRGBS without a ton of typing
        'A function to get the RGB value back from a string such as _RGB32(255,255,255)
        'text$ is the string that we send to check for a value
        'DefaultColor is the value we send back if the string isn't in the proper format

        VRGBS~& = DefaultColor 'A return the default value if we can't parse the string properly
        If UCase$(Left$(text$, 4)) = "_RGB" Then
            rpos = InStr(text$, "(")
            gpos = InStr(rpos, text$, ",")
            bpos = InStr(gpos + 1, text$, ",")
            If rpos <> 0 And bpos <> 0 And gpos <> 0 Then
                red = Val(_Trim$(Mid$(text$, rpos + 1)))
                green = Val(_Trim$(Mid$(text$, gpos + 1)))
                blue = Val(_Trim$(Mid$(text$, bpos + 1)))
                VRGBS~& = _RGB32(red, green, blue)
            End If
        End If
    End Function
    '$INCLUDE:'.\ini_manager.bm'
$End If
