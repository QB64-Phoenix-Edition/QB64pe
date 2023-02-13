$Console:Only

On Timer(2) GoSub timerhand
Timer On

' The timer triggering should end sleep early, so it only triggers once
Sleep 10

System

timerhand:
Print "Timer!"
return
