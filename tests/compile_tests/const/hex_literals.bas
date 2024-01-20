$CONSOLE:ONLY

CONST test__byte%% = &H4F
CONST test__byte_neg%% = &HFF

CONST test__integer% = &H00FF
CONST test__integer_neg% = &HFFF3

CONST test__long& = &H00FF00FF
CONST test__long_neg& = &HFFFFFF00 ' -256
CONST test__long_neg16& = &HFF00 ' -256

CONST test__ulong~& = &H4FFFFF00
CONST test__ulong_neg~& = &HFFFFFF00
CONST test__ulong_neg16~& = &HFF00 ' sign extension to &HFFFFFF00~&

CONST test__int64&& = &H4FFFFFFFFFFFFF00
CONST test__int64_neg&& = &HFFFFFFFFFFFFFF00
CONST test__int64_neg32&& = &HFFFFFF00 ' sign extension to &HFFFFFFFFFFFFFF00
CONST test__int64_neg16&& = &HFF00 ' sign extension to &HFFFFFFFFFFFFFF00

CONST test__uint64_neg~&& = &HFFFFFFFFFFFFFF00
CONST test__uint64_neg32~&& = &HFFFFFF00 ' sign extension to &HFFFFFFFFFFFFFF00
CONST test__uint64_neg64~&& = &HFFFFFFFFFFFFFF00

CONST test__uint64&& = &H4000000000000001

CONST test__ulong_uinteger~& = &HFFFF~% ' should avoid sign extension due to unsigned type

PRINT "byte: "; test__byte%%; HEX$(test__byte%%) ' 4F
PRINT "byte negative: "; test__byte_neg%%; HEX$(test__byte_neg%%) ' FF

PRINT "integer: "; test__integer%; HEX$(test__integer%) ' FF
PRINT "integer negative: "; test__integer_neg%; HEX$(test__integer_neg%) ' FFF3

PRINT "long: "; test__long&; HEX$(test__long&) ' FF00FF
PRINT "long negative: "; test__long_neg&; HEX$(test__long_neg&) ' FFFFFF00
PRINT "long negative sign extension: "; test__long_neg16&; HEX$(test__long_neg16&) ' FFFFFF00

PRINT "ulong: "; test__ulong~&; HEX$(test__ulong~&) ' 4FFFFF00
PRINT "ulong negative: "; test__ulong_neg~&; HEX$(test__ulong_neg~&) ' FFFFFF00
PRINT "ulong negative sign extension: "; test__ulong_neg16~&; HEX$(test__ulong_neg16~&) ' FFFFFF00

PRINT "int64: "; test__int64&&; HEX$(test__int64&&) ' 4FFFFFFFFFFFFF00
PRINT "int64 negative: "; test__int64_neg&&; HEX$(test__int64_neg&&) ' FF00?
PRINT "int64 negative sign extension: "; test__int64_neg32&&; HEX$(test__int64_neg32&&) ' FF00?
PRINT "int64 negative sign extension: "; test__int64_neg16&&; HEX$(test__int64_neg16&&) ' FFFFFF00?

PRINT "uint64 negative: "; test__uint64_neg~&&; HEX$(test__uint64_neg~&&) ' FF00 - possible bug
PRINT "uint64 negative sign extension: "; test__uint64_neg32~&&; HEX$(test__uint64_neg32~&&) ' FF00 - possible bug
PRINT "uint64 negative sign extension: "; test__uint64_neg64~&&; HEX$(test__uint64_neg64~&&) ' FF00 - possible bug

PRINT "uint64: "; test__uint64&&; HEX$(test__uint64&&) ' 4000000000000001

PRINT "ulong uinteger: "; test__ulong_uinteger~&; HEX$(test__ulong_uinteger~&) ' 4000000000000001

SYSTEM
