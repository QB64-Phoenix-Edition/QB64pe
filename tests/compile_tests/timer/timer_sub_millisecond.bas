$Console:Only

' TIMER with an accuracy finer than 1 ms returns microseconds; .001 and
' coarser keep returning whole milliseconds, as they always have.

Dim As Double t, prev, msPart
Dim As Long i, offMs, backwards, notWholeMs

' TIMER(.001) is always a whole number of milliseconds
For i = 1 To 2000
    t = Timer(.001)
    msPart = t * 1000 - Int(t * 1000 + .5)
    If Abs(msPart) > .000001 Then notWholeMs = notWholeMs + 1
Next
If notWholeMs = 0 Then Print "Timer(.001) whole milliseconds: ok" Else Print "Timer(.001) not whole milliseconds:"; notWholeMs

' TIMER(.000001) has digits below the millisecond, and never runs backwards
prev = Timer(.000001)
For i = 1 To 20000
    t = Timer(.000001)
    msPart = t * 1000 - Int(t * 1000 + .5)
    If Abs(msPart) > .0005 Then offMs = offMs + 1
    If t < prev And prev - t < 43200 Then backwards = backwards + 1 ' (ignores the midnight wrap)
    prev = t
Next
If offMs > 0 Then Print "Timer(.000001) below a millisecond: ok" Else Print "Timer(.000001) only whole milliseconds"
If backwards = 0 Then Print "Timer(.000001) never backwards: ok" Else Print "Timer(.000001) went backwards:"; backwards

' Both agree to within a couple of milliseconds
t = Abs(Timer(.000001) - Timer(.001))
If t < .002 Or t > 86399 Then Print "Timer(.000001) agrees with Timer(.001): ok" Else Print "Timer(.000001) differs from Timer(.001) by"; t

System
