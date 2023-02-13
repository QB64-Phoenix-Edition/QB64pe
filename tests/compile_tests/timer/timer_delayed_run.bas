$Console:Only

On Timer(2) GoSub timerhand

' This first delay should not matter
_Delay 3
Timer On

' The timer should be triggered twice
_Delay 5
System

timerhand:
Print "Timer!"
return
