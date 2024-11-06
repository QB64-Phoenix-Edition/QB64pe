$Console:Only

h& = _OpenClient("https://www.example.com")
Print h&

length~& = LOF(h&)

result$ = ""
While Not Eof(h&)
    _Limit 100
    GET #h&, , s$
    result$ = result$ + s$
Wend


' Strip off the trailing slash if it's there to make the result consistent
url$ = _ConnectionAddress$(h&)
If MID$(url$, LEN(url$), 1) = "/" Then
    url$ = Left$(Url$, LEN(url$) - 1)
End If

Print "Content-Length: "; length~&
Print "Url: "; url$
Print "Status Code: "; _StatusCode(h&)
Print
Print result$

Close h&

System
