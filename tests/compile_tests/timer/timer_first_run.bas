$Console:Only

On Timer(2) GoSub timerhand
Timer On

' Timer should be triggered twice
_Delay 5
System

timerhand:
Print "Timer!"
return
