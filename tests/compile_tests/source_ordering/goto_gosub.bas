'Tests GOTO and GOSUB behave as expected
$Console:Only

label1:
s
GoTo skip

top:
t
Return

Sub s
    Print "Start"
    GoSub s_g
    Exit Sub

    s_g:
    Print "Start's gosub"
    Return
End Sub

skip:
If x = 0 Then
    x = 1
    GoTo label1
End If

Sub t
    Print "Top"
End Sub

GoSub top
GoSub bottom
Print "End"
System

bottom:
Print "Bottom"
Return
