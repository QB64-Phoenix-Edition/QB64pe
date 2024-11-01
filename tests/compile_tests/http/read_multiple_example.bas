$Console:Only
Option Base 0

Dim handles&(99)
Dim Content$(LBound(handles&) To UBound(handles&))

For x = LBound(handles&) TO UBound(handles&)
    ' The math here "randomizes" the order of the handles in the array, while
    ' still keeping the result predictable. The numbers are picked so that
    ' every entry 0 to 99 still gets filled in.
    handles&((x * 3) Mod (UBound(handles&) + 1)) = _OpenClient("https://www.example.com")
Next

' Read from all the connections in parallel
Done& = 0
While Not Done&
    _LIMIT 100
    Done& = -1

    For x = LBound(handles&) To UBound(handles&)
        Get #handles&(x), , s$
        content$(x) = content$(x) + s$

        Done& = Done& And Eof(handles&(x))
    Next
Wend

For x = LBound(handles&) TO UBound(handles&)
    Print "Handle:"; handles&(x); ", LOF:"; Lof(handles&(x)); ", Content length:"; Len(content$(x)); ", Status Code: "; _StatusCode(handles&(x))

    Close #handles&(x)
Next

System
