$Console:Only

On Timer(2) GoSub timerhand
Timer On
Timer Stop

' Timer will not trigger when stopped
Sleep 3

' Timer should trigger immediately when started as two seconds have elapsed
' while it was stopped
Timer On
Timer Off 'Shouldn't matter, timer triggers as soon as Timer On runs
System

timerhand:
Print "Timer!"
return
