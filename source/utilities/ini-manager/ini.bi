'INI Manager v2.0
'Fellippe Heitor, 2017-2026 - @fellippeheitor

'Include this file at the top of your program (after OPTION _EXPLICIT, if used).
'
'It is optional, but recommended:
' - it is required if you use OPTION _EXPLICIT;
' - with QB64-PE 3.14.1 or newer, it lets the library report file errors
'   (read-only files, missing folders, locked files...) through IniCODE
'   instead of stopping your program with a runtime error.

$LET INI_BI = 1
$IF VERSION >= 3.14.1 THEN
    $LET INI_SAFEIO = 1
$END IF

'Public variables
DIM IniCODE
DIM IniLastSection$
DIM IniLastKey$
DIM IniDisableAutoCommit
DIM IniAllowBasicComments
DIM IniForceReload
DIM IniDisableAddQuotes

'Internal state
DIM IniCurrentFile$
DIM IniWholeFile$
DIM IniLF$
DIM IniLoaded%%
DIM IniDirty%%
DIM IniDocVersion&
DIM IniIterSig$, IniIterPos&, IniIterCount&, IniIterVersion&, IniIterSect$
DIM IniSectSig$, IniSectPos&, IniSectCount&, IniSectVersion&
DIM IniIOError&

'Error trap used by the library while it reads or writes files.
'Your own ON ERROR handler is saved and restored around every file operation.
GOTO IniSkipErrorHandler
IniIOErrorHandler:
IniIOError& = ERR
RESUME NEXT
IniSkipErrorHandler:
