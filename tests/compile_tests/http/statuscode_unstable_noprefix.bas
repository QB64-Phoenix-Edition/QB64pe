$NOPREFIX
$Unstable:Http

' Important so that `StatusCode()` cannot be an array, ensures that this test
' fails if `StatusCode()` cannot be called
Option ExplicitArray

Print StatusCode(2)
