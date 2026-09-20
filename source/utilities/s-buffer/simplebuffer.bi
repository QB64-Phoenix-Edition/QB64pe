'+---------------+---------------------------------------------------+
'| ###### ###### |     .--. .         .-.                            |
'| ##  ## ##   # |     |   )|        (   ) o                         |
'| ##  ##  ##    |     |--' |--. .-.  `-.  .  .-...--.--. .-.        |
'| ######   ##   |     |  \ |  |(   )(   ) | (   ||  |  |(   )       |
'| ##      ##    |     '   `'  `-`-'  `-'-' `-`-`|'  '  `-`-'`-      |
'| ##     ##   # |                            ._.'                   |
'| ##     ###### |  Sources & Documents placed in the Public Domain. |
'+---------------+---------------------------------------------------+
'|                                                                   |
'| === simplebuffer.bi ===                                           |
'|                                                                   |
'| == Definitions required for the routines in simplebuffer.bm.      |
'|                                                                   |
'+-------------------------------------------------------------------+
'| Done by RhoSigma, R.Heyder, provided AS IS, use at your own risk. |
'| Find me in the QB64 Forum or mail to support@rhosigma-cw.net for  |
'| any questions or suggestions. Thanx for your interest in my work. |
'+-------------------------------------------------------------------+
'
'To optimize for how QB64-PE makes use of the library some parts of it are
'omitted.

'--- The internal array for data storage
'-----
'never access this directly, use functions in simplebuffer.bm
'
'Each buffer occupies simplebuffer_slots consecutive strings:
'  +0  the buffer data
'  +1  cursor position, buffer length, EOL mode and change state
CONST simplebuffer_slots = 2
REDIM SHARED simplebuffer_array$(0 TO 100 * simplebuffer_slots - 1)

'--- Bookkeeping so that CreateBuf/DisposeBuf don't have to walk the array
'-----
'
' simplebuffer_freeHint& is the lowest buffer base which may still be free, i.e.
' every buffer below it is known to be in use.
'
' simplebuffer_topUsed& is an upper bound for the highest buffer base in use,
' so only disposing that buffer can open up a new opportunity to shrink.
DIM SHARED simplebuffer_freeHint&, simplebuffer_topUsed&
simplebuffer_freeHint& = 0
simplebuffer_topUsed& = -simplebuffer_slots

'--- The EndOfLine sequences for each platform.
'-----
DIM SHARED simplebuffer_winEol$, simplebuffer_lnxEol$, simplebuffer_nativeEol$
simplebuffer_winEol$ = CHR$(13) + CHR$(10)
simplebuffer_lnxEol$ = CHR$(10)
IF INSTR(_OS$, "[LINUX]") > 0 THEN 'true for MacOSX too
    simplebuffer_nativeEol$ = simplebuffer_lnxEol$
ELSE
    simplebuffer_nativeEol$ = simplebuffer_winEol$
END IF

'--- Simplebuffer Errors (most FUNCTIONs)
'-----
'initializer error returns
CONST SBE_NoMoreBuffers = -1
'operational error returns
CONST SBE_UnknownMode = -11
CONST SBE_OutOfBounds = -12

'--- Simplebuffer Modes (SeekBuf) ---
'-----
'use for mode% argument
CONST SBM_PosRestore = -21
CONST SBM_BufStart = -22
CONST SBM_BufCurrent = -23
CONST SBM_BufEnd = -24
CONST SBM_LineStart = -25
CONST SBM_LineEnd = -26

'$INCLUDE: 'sb_qb64pe_extension.bi'

