'String SPacer/delimiter constants
'sp is used as the primary string spacer
'sp2 & sp3 are used when further delimiation is required
'for instance, sp2 is used for embedding spacing info for auto-layout by an IDE
DIM SHARED sp AS STRING * 1, sp2 AS STRING * 1, sp3 AS STRING * 1
sp = _CHR_CR: sp2 = _CHR_LF: sp3 = _CHR_SUB
DIM SHARED sp_asc AS LONG, sp2_asc AS LONG, sp3_asc AS LONG
sp_asc = _ASC_CR: sp2_asc = _ASC_LF: sp3_asc = _ASC_SUB
IF Debug THEN sp = CHR$(250): sp2 = CHR$(249): sp3 = CHR$(179) 'makes debug output more readable

DIM SHARED CHR_QUOTE AS STRING * 1: CHR_QUOTE = _CHR_QUOTE
DIM SHARED CHR_TAB AS STRING * 1: CHR_TAB = _CHR_HT 'horizontal tab
DIM SHARED CRLF AS STRING: CRLF = _STR_CRLF 'carriage return + line feed

DIM SHARED NATIVE_LINEENDING AS STRING
IF INSTR(_OS$, "WIN") THEN NATIVE_LINEENDING = _STR_CRLF ELSE NATIVE_LINEENDING = _STR_LF

DIM SHARED OS_BITS AS LONG: OS_BITS = 64
IF INSTR(_OS$, "[32BIT]") THEN OS_BITS = 32

