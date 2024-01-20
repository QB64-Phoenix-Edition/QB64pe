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

Dim tests(10) As String
tests(1) = "2@+@+@+@3"
tests(2) = "2@-@-@3"
tests(3) = "2@-@+@3@-@-@4"
tests(4) = "(@-@-@3@+@+@3@)@-@-@3"
tests(5) = "-@-@3"
tests(6) = "-@+@3"
tests(7) = "+@-@3"
tests(8) = "+@+@3"
tests(9) = "+@+@+@3"
tests(10) = "-@-@-@3"

For i = 1 TO UBOUND(tests)
    Print "Test: "; Readable$(tests(i))
    Print "DWD: "; Readable$(DWD$(tests(i)))
Next i

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
