'Tests all of main procedure before subprocedures
$Console:Only

s
Print f$
System

Sub s
    Print "sub s"
End Sub

Function f$
    f$ = "function f"
End Function
