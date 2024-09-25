$UNSTABLE:HTTP

' Important so that `StatusCode()` cannot be an array, ensures that this test
' fails if `_StatusCode()` cannot be called
OPTION _EXPLICITARRAY

PRINT _STATUSCODE(2)

