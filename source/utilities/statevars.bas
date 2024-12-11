'Routines to work with RCStateVars. These variables are used to track the
'activation of features and the possibly required recompiles for it.
'Please do not manually manipulate any elements of a RCStateVar, always
'use these routines to ensure the integrity of the tracking system.

'Clear/Reset all elements of the given state variable.
'>> to be used between labels fullrecompile: and recompile: in qb64pe.bas
SUB ClearRCStateVar (stVar AS RCStateVar)
    stVar.wanted = 0: stVar.actual = 0
    stVar.locked = 0: stVar.forced = 0
END SUB

'Set a new state value in the given state variable. A recompile is
'triggered automatically at the end of the current pass, if required.
'>> to be used whenever the state is changed e.g. by metacommands
SUB SetRCStateVar (stVar AS RCStateVar, setVal%%)
    stVar.wanted = setVal%%
    IF stVar.actual <> stVar.wanted AND stVar.locked = _FALSE THEN
        recompile = 1 'recompile trigger
    END IF
END SUB

'Force the given state value, overrides the set value at variable execution
'time, i.e. at the beginning of each (re)compile.
'>> to be used whenever special conditions require it
SUB ForceRCStateVar (stVar AS RCStateVar, forceVal%%)
    stVar.forced = forceVal%%
END SUB

'Executes the pending state changes in the given state variable.
'>> to be used right after the recompile: label in qb64pe.bas
SUB ExecuteRCStateVar (stVar AS RCStateVar)
    IF stVar.locked = _FALSE THEN
        stVar.actual = stVar.wanted
        IF stVar.actual <> 0 THEN stVar.locked = _TRUE
    END IF
    IF stVar.forced <> 0 THEN stVar.actual = stVar.forced
END SUB

'Return the actual value of the given state variable.
'>> to be used whenever the value needs to be checked/retrieved
FUNCTION GetRCStateVar%% (stVar AS RCStateVar)
    GetRCStateVar%% = stVar.actual
END FUNCTION

