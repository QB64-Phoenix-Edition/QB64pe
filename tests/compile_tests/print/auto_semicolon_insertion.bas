$console:only

'This test confirms PRINT automatically inserts ; on either side
'of a string literal.

'Disable formatting otherwise the IDE would ruin this test by inserting ;
'$Format:Off

'Cases of semicolon insertion:

'With numeric literals
Print 123"abc"
Print "abc"123
Print .23"abc"
Print "abc".23
Print 23e+"abc"
Print 23e-"abc"
Print "abc"&H10

'Sigils
Print 12%"abc"
x$ = "hi": Print x$"abc"

'Variables
n = 12
Print n"abc"
Print "abc"n

'Adjacent function
Print Int(1.1)"abc"
Print "abc"Int(1.1)

'Multiple on one line
Print "abc""def"23


'Cases where semicolon insertion is prohibited:

'Concatenation
Print "abc"+"def"
Print _ToStr$(2)+"def"

'Comparison
Print "abc">"def"
Print "abc"<"def"
Print "abc">="def"
Print "abc"<="def"
Print "abc"=>"def"
Print "abc"=<"def"
Print "abc"="def"
Print "abc"<>"def"

'Parentheses
Print ("abc") + "def"
Print Len("foo")

'Separator already present
Print "abc" ; "def"
'Print "abc" , "def" 'disabled as it screws console output on windows an makes the test hang

'Specific reported failing case
Print Int("hello" <> "world") 

System