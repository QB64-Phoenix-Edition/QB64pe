DEFLNG A-Z
$Console:Only

Dim Debug As Long

'$include:'../../../source/global/constants.bas'
sp = "@" ' Makes sequences easier to write

'$include:'../../../source/utilities/const_eval.bi'
'$include:'../../../source/utilities/ini-manager/ini.bi'
'$include:'../../../source/utilities/s-buffer/simplebuffer.bi'
'$include:'../../../source/utilities/hash.bi'
'$include:'../../../source/utilities/type.bi'
'$include:'../../../source/utilities/give_error.bi'

Dim test As String

test = CHR$(34) + "foobar" + CHR$(34) + ",6"

Dim num As ParseNum, eval AS String

eval = Evaluate_Expression$(test, num)

Print "eval result: " + eval
Print "num.s: " + num.s


test = test + "@+@" + test

eval = Evaluate_Expression$(test, num)

Print "eval result: " + eval
Print "num.s: " + num.s

test = test + "@+@" + CHR$(34) + "test\034" + CHR$(34) + ",5"

eval = Evaluate_Expression$(test, num)

Print "eval result: " + eval
Print "num.s: " + num.s

SYSTEM

'$include:'../../../source/utilities/ini-manager/ini.bm'
'$include:'../../../source/utilities/s-buffer/simplebuffer.bm'
'$include:'../../../source/utilities/elements.bas'
'$include:'../../../source/utilities/const_eval.bas'
'$include:'../../../source/utilities/hash.bas'
'$include:'../../../source/utilities/give_error.bas'
'$include:'../../../source/utilities/strings.bas'
'$include:'../../../source/utilities/type.bas'
