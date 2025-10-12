$CONSOLE:ONLY
OPTION BASE 0

DIM handles&(99)
DIM Content$(LBOUND(handles&) TO UBOUND(handles&))

FOR x = LBOUND(handles&) TO UBOUND(handles&)
    ' The math here "randomizes" the order of the handles in the array, while
    ' still keeping the result predictable. The numbers are picked so that
    ' every entry 0 to 99 still gets filled in.
    handles&((x * 3) MOD (UBOUND(handles&) + 1)) = _OPENCLIENT("https://www.qb64phoenix.com")
NEXT

' Read from all the connections in parallel
Done& = 0
WHILE NOT Done&
    _LIMIT 100
    Done& = -1

    FOR x = LBOUND(handles&) TO UBOUND(handles&)
        GET #handles&(x), , s$
        Content$(x) = Content$(x) + s$

        Done& = Done& AND EOF(handles&(x))
    NEXT
WEND

FOR x = LBOUND(handles&) TO UBOUND(handles&)
    PRINT "Handle: "; handles&(x); ", Match: "; _IIF(LOF(handles&(x)) = LEN(Content$(x)), "yes", "no"); ", Status Code: "; _STATUSCODE(handles&(x))

    CLOSE #handles&(x)
NEXT

SYSTEM
