DEFLNG A-Z
$Console:Only

Dim Shared E

Type TestCase
    file As String
    expectedExtension As String
End Type

Dim tests(5) As TestCase

tests(1).file = "foobar.exe"
tests(1).expectedExtension = "exe"

tests(2).file = "foobar.EXE"
tests(2).expectedExtension = "EXE"

tests(3).file = "foobar."
tests(3).expectedExtension = ""

tests(4).file = "foobar"
tests(4).expectedExtension = ""

tests(5).file = "foobar.tar.gz"
tests(5).expectedExtension = "gz"

For i = 1 To UBOUND(tests)
    result$ = GetFileExtension$(tests(i).file)

    Print "Test"; i; ", Filename: "; tests(i).file
    Print "    Expected: "; tests(i).expectedExtension; ", Actual: "; result$

    If result$ = tests(i).expectedExtension Then
        Print "      PASS!"
    Else
        Print "      FAIL!"
    End If
Next

done:
System

'These error handlers are not used in this progam, but are required
'for the functions included from file.bas
qberror:
Print "      FAIL!"
Resume done

qberror_test:
E = 1
Resume Next

'$include:'../../../source/utilities/file.bas'
