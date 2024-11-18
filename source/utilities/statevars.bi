'Defining the RCStateVar type. These variables are used to track the
'activation of features and the possibly required recompiles for it.
'Please do not manually manipulate any elements of a RCStateVar, always
'use the routines provided in statevars.bas to ensure the integrity of
'the tracking system.

TYPE RCStateVar
    wanted AS _BYTE 'last set value
    actual AS _BYTE 'currently active value
    locked AS _BYTE 'recompile done, value locked
    forced AS _BYTE 'forced override of locked value
END TYPE

