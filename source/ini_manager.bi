'INI Manager
'Fellippe Heitor, 2017-2021 - fellippe@qb64.org - @fellippeheitor

'This file isn't required to be at the top of your programs,
'unless you intend to use OPTION _EXPLICIT

'Global variables declaration
$If INI_BI = UNDEFINED Then
    Dim currentIniFileName$
    Dim currentIniFileLOF As _Unsigned Long
    Dim IniWholeFile$
    Dim IniSectionData$
    Dim IniPosition As _Unsigned Long
    Dim IniNewFile$
    Dim IniLastSection$
    Dim IniLastKey$
    Dim IniLF$
    Dim IniDisableAutoCommit
    Dim IniCODE
    Dim IniAllowBasicComments
    Dim IniForceReload
    Dim IniDisableAddQuotes
$End If
$Let INI_BI = TRUE
