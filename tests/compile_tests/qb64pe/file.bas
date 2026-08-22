DEFLNG A-Z
$Console:Only

Dim Shared E

'file.bas resolves paths against these. We drive them explicitly rather than
'detecting the host, so the expected output is the same on every platform.
Dim Shared os As String
Dim Shared pathsep As String * 1

Type TestCase
    file As String
    expectedExtension As String
End Type

Type RelativeCase
    sep As String * 1
    baseDir As String
    target As String
    expected As String
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

Dim relTests(7) As RelativeCase

'target sits directly in the base directory
relTests(1).sep = "/"
relTests(1).baseDir = "/home/user/proj/"
relTests(1).target = "/home/user/proj/test.bas"
relTests(1).expected = "test.bas"

'target sits below the base directory
relTests(2).sep = "/"
relTests(2).baseDir = "/home/user/proj/"
relTests(2).target = "/home/user/proj/lib/helper.bm"
relTests(2).expected = "lib/helper.bm"

'target is a sibling of the base directory, so we have to walk up once
relTests(3).sep = "/"
relTests(3).baseDir = "/home/user/proj/"
relTests(3).target = "/home/user/shared/x.bi"
relTests(3).expected = "../shared/x.bi"

'walking up several levels
relTests(4).sep = "/"
relTests(4).baseDir = "/home/user/proj/a/b/"
relTests(4).target = "/home/user/x.bi"
relTests(4).expected = "../../../x.bi"

'a base without a trailing separator is still a directory
relTests(5).sep = "/"
relTests(5).baseDir = "/home/user/proj"
relTests(5).target = "/home/user/proj/test.bas"
relTests(5).expected = "test.bas"

'backslash separators
relTests(6).sep = "\"
relTests(6).baseDir = "C:\proj\"
relTests(6).target = "C:\proj\lib\helper.bm"
relTests(6).expected = "lib\helper.bm"

'no shared ancestor at all, so there's nothing to be relative to and the
'absolute path is handed back unchanged
relTests(7).sep = "\"
relTests(7).baseDir = "C:\proj\"
relTests(7).target = "D:\other\x.bi"
relTests(7).expected = "D:\other\x.bi"

For i = 1 To UBOUND(relTests)
    pathsep$ = relTests(i).sep
    If pathsep$ = "\" Then os$ = "WIN" Else os$ = "LNX"

    result$ = MakeRelativePath$(relTests(i).baseDir, relTests(i).target)

    Print "Relative test "; _ToStr$(i)
    Print "    Base: "; relTests(i).baseDir; ", Target: "; relTests(i).target
    Print "    Expected: "; relTests(i).expected; ", Actual: "; result$

    If result$ = relTests(i).expected Then
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
