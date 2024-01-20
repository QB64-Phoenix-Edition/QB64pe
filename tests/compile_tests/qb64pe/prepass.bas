DEFLNG A-Z
$Console:Only

Dim Debug As Long

'$include:'../../../source/global/constants.bas'
sp = "@" ' Makes the output readable

'$include:'../../../source/utilities/const_eval.bi'
'$include:'../../../source/utilities/ini-manager/ini.bi'
'$INCLUDE:'../../../source/utilities/s-buffer/simplebuffer.bi'
'$include:'../../../source/utilities/hash.bi'
'$include:'../../../source/utilities/type.bi'
'$include:'../../../source/utilities/give_error.bi'

Dim tests(4) As String
' These tests cover the paren insert around NOT, and some simple cases
tests(1) = "(@20@+@40@+@(@60@*@4@AND@50@+@NOT@5@+@4@)@-@2@)"
tests(2) = "(@20@+@40%@+@60000000&&@+@_RGB32@(@20@,@50@,@60@)@+@(@60@*@4@AND@50@+@NOT@5@+@4@)@-@2@)"
tests(3) = "2@+@NOT@5@+@2@*@6@^@3"
tests(4) = "2@+@-@2@"

For i = 1 TO UBOUND(tests)
    Print "Test: "; Readable$(tests(i))
    PreParse tests(i)
    Print "PrePass: "; Readable$(tests(i))
Next i

' Test empty string
test2$ = ""
Print "Test: "; Readable$(test2$)
PreParse test2$
Print "PrePass: "; Readable$(test2$)

Dim errs(5) As String

' Various invalid paren cases
errs(1) = ")@("
errs(2) = "(@(@)@)@)"
errs(3) = "(@(@(@)@)"
errs(4) = "("
errs(5) = ")"

For i = 1 to UBOUND(errs)
	Print "Test: "; Readable$(errs(i))
	PreParse errs(i)
	Print "PrePass: "; Readable$(errs(i))
Next

SYSTEM

'$include:'../../../source/utilities/ini-manager/ini.bm'
'$include:'../../../source/utilities/s-buffer/simplebuffer.bm'
'$include:'../../../source/utilities/elements.bas'
'$include:'../../../source/utilities/const_eval.bas'
'$include:'../../../source/utilities/hash.bas'
'$include:'../../../source/utilities/give_error.bas'
'$include:'../../../source/utilities/strings.bas'
'$include:'../../../source/utilities/type.bas'

FUNCTION Readable$(a$)
	r$ = ""

	FOR i = 1 TO numelements(a$)
		r$ = r$ + getelement$(a$, i) + " "
	NEXT

	Readable$ = r$
END FUNCTION
