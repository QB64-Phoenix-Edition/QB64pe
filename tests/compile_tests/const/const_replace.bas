$CONSOLE:ONLY

CONST const__OR__test = 1 OR 2 ' 3
CONST const__AND__test = 3 AND 1 ' 1
CONST const__XOR__test = 2 XOR 3 ' 1
CONST const__mod__test = 20 MOD 3 ' 2

CONST const__byte%% = 1%%
CONST const__ubyte~%% = 2~%%
CONST const__int% = 4%
CONST const__uint~% = 8~%
CONST const__long& = 16&
CONST const__ulong~& = 32~&
CONST const__int64&& = 64&&
CONST const__uint64~&& = 128~&&
CONST const__single! = 256!
CONST const__double# = 512#
CONST const__float## = 1024##

CONST const__negative = -20000

' Test original casing, and UCASE
CONST const_replace_test = const__OR__test + const__AND__test + const__XOR__test + const__mod__test
CONST const_replace_test_ucase = CONST__OR__test + CONST__AND__test + CONST__XOR__test + CONST__MOD__test

' Defined with suffix, used with suffix
CONST const_replace_test_suffix = const__byte%% + const__ubyte~%% + const__int% + const__uint~% + const__long& + const__ulong~& + const__int64&& + const__uint64~&& + const__single! + const__double# + const__float##

' Defined with suffix, but missing when used
CONST const_replace_test_no_suffix = const__byte + const__ubyte + const__int + const__uint + const__long + const__ulong + const__int64 + const__uint64 + const__single + const__double + const__float


CONST const_replace_test_added_suffix = const__OR__test&& + const__AND__test&& + const__XOR__test&& + const__mod__test&&

CONST const_replace_test_negative = 20 - const__negative

PRINT const_replace_test
PRINT const_replace_test_ucase
PRINT const_replace_test_suffix
PRINT const_replace_test_no_suffix
PRINT const_replace_test_added_suffix
PRINT const_replace_test_negative

SYSTEM
