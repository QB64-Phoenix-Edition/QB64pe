DIM SHARED AS LONG IDEAutoLayout, IDEAutoLayoutKwCapitals, IDEAutoIndent, IDEAutoIndentSize, IDEIndentSubs
DIM SHARED IDECommentColor AS _UNSIGNED LONG, IDEMetaCommandColor AS _UNSIGNED LONG
DIM SHARED IDEQuoteColor AS _UNSIGNED LONG, IDETextColor AS _UNSIGNED LONG
DIM SHARED IDEBackgroundColor AS _UNSIGNED LONG, IDEChromaColor AS _UNSIGNED LONG
DIM SHARED IDEBackgroundColor2 AS _UNSIGNED LONG, IDEBracketHighlightColor AS _UNSIGNED LONG
DIM SHARED IDEKeywordColor AS _UNSIGNED LONG, IDENumbersColor AS _UNSIGNED LONG
DIM SHARED IDEErrorColor AS _UNSIGNED LONG
DIM SHARED IDE_AutoPosition AS _BYTE, IDE_TopPosition AS INTEGER, IDE_LeftPosition AS INTEGER
DIM SHARED IDE_BypassAutoPosition AS _BYTE, idesortsubs AS _BYTE, IDESubsLength AS _BYTE
DIM SHARED IDENormalCursorStart AS LONG, IDENormalCursorEnd AS LONG
DIM SHARED MouseButtonSwapped AS _BYTE
DIM SHARED PasteCursorAtEnd AS _BYTE
DIM SHARED SaveExeWithSource AS _BYTE, EnableQuickNav AS _BYTE
DIM SHARED IDEShowErrorsImmediately AS _BYTE
DIM SHARED ShowLineNumbersSeparator AS _BYTE, ShowLineNumbersUseBG AS _BYTE
DIM SHARED IgnoreWarnings AS _BYTE, qb64versionprinted AS _BYTE
DIM SHARED DisableSyntaxHighlighter AS _BYTE, ExeToSourceFolderFirstTimeMsg AS _BYTE
DIM SHARED WhiteListQB64FirstTimeMsg AS _BYTE
DIM SHARED WatchListToConsole AS _BYTE
DIM SHARED windowSettingsSection$, colorSettingsSection$, customDictionarySection$
DIM SHARED mouseSettingsSection$, generalSettingsSection$, displaySettingsSection$
DIM SHARED colorSchemesSection$, debugSettingsSection$, compilerSettingsSection$, vwatchPanelSection$
DIM SHARED ConfigFolder$, askToCopyOther AS _BYTE
DIM SHARED ConfigFile$, DebugFile$, AutosaveFile$, RecentFile$, SearchedFile$, BookmarksFile$, UndoFile$
DIM SHARED idebaseTcpPort AS LONG, AutoAddDebugCommand AS _BYTE
DIM SHARED wikiBaseAddress$
DIM SHARED MaxParallelProcesses AS LONG
DIM SHARED ExtraCppFlags AS STRING, ExtraLinkerFlags AS STRING
DIM SHARED StripDebugSymbols AS LONG
DIM SHARED OptimizeCppProgram AS LONG
DIM SHARED GenerateLicenseFile AS LONG
DIM SHARED UseGuiDialogs AS _UNSIGNED LONG
DIM SHARED DefaultTerminal AS STRING
DIM SHARED LoggingEnabled AS _UNSIGNED LONG

'===== Define and check settings location =====================================
ConfigFolder$ = "settings" 'relative config location inside the qb64pe main folder
ConfigFile$ = ConfigFolder$ + pathsep$ + "config.ini" 'main configuration (global with instance sections)
DebugFile$ = ConfigFolder$ + pathsep$ + "debug.ini" 'debug mode data (global with instance sections)
BookmarksFile$ = ConfigFolder$ + pathsep$ + "bookmarks.bin" 'set bokmarks (globally tied to source files)
RecentFile$ = ConfigFolder$ + pathsep$ + "recent.bin" 'recent file list (globally shared)
SearchedFile$ = ConfigFolder$ + pathsep$ + "searched.bin" 'search history (globally shared)
AutosaveFile$ = ConfigFolder$ + pathsep$ + "autosave" + tempfolderindexstr$ + ".bin" 'autosave flag (per instance)
UndoFile$ = ConfigFolder$ + pathsep$ + "undo" + tempfolderindexstr$ + ".bin" 'undo storage (per instance)
'---
askToCopyOther = 0 'shall we ask the user to copy settings from another QB64-PE installation
IF NOT _DIREXISTS(ConfigFolder$) THEN MKDIR ConfigFolder$: askToCopyOther = -1

'===== Define sections and standard behavior ==================================
'--- config.ini
windowSettingsSection$ = "IDE WINDOW" + STR$(tempfolderindex)
colorSettingsSection$ = "IDE COLOR SETTINGS" + STR$(tempfolderindex)
colorSchemesSection$ = "IDE COLOR SCHEMES"
customDictionarySection$ = "CUSTOM DICTIONARIES"
mouseSettingsSection$ = "MOUSE SETTINGS"
generalSettingsSection$ = "GENERAL SETTINGS"
displaySettingsSection$ = "IDE DISPLAY SETTINGS"
debugSettingsSection$ = "DEBUG SETTINGS"
compilerSettingsSection$ = "COMPILER SETTINGS"
'--- debug.ini
vwatchPanelSection$ = "VWATCH PANEL" + STR$(tempfolderindex)
'--- behavior
IniSetAddQuotes 0
IniSetForceReload -1
IniSetAllowBasicComments -1
IniSetAutoCommit -1

