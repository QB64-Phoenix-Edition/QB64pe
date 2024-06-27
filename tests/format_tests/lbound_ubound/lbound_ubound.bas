dim Foo(1 to 3)
print lbound(foo) - _pixelsize - 125
print ubound(FOO, lbound(foo)) - 1
Dim BAR As Long
Dim Bar(1 To 3)
print lbound(bar) - _pixelsize - 125
print ubound(BAR, lbound(bar)) - 1 + bar