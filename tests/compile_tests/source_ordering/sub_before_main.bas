'Tests all of main procedure after subprocedures
$Console:Only

Sub s
    Print "sub s"
End Sub

Function f$
    f$ = "function f"
End Function

s
Print f$
System
