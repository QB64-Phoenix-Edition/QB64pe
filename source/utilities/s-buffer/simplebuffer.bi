'+---------------+---------------------------------------------------+
'| ###### ###### |     .--. .         .-.                            |
'| ##  ## ##   # |     |   )|        (   ) o                         |
'| ##  ##  ##    |     |--' |--. .-.  `-.  .  .-...--.--. .-.        |
'| ######   ##   |     |  \ |  |(   )(   ) | (   ||  |  |(   )       |
'| ##      ##    |     '   `'  `-`-'  `-'-' `-`-`|'  '  `-`-'`-      |
'| ##     ##   # |                            ._.'                   |
'| ##     ###### |  Sources & Documents placed in the Public Domain. |
'+---------------+---------------------------------------------------+

'--- The internal array for data storage
'-----
'never access this directly, use functions in simplebuffer.bm
REDIM SHARED simplebuffer_array$(0 TO 10599) 'init for 100 buffers

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
CONST SBM_BufStart = -22
CONST SBM_BufCurrent = -23
CONST SBM_BufEnd = -24

'$INCLUDE: 'sb_qb64pe_extension.bi'

