# The QB64-PE auto-include logic
We have a nice auto-including feature since **QB64-PE v4.0.0**, which makes sure that "under the hood" support files (general and color CONSTs, vwatch code ($DEBUG) and other support code) are included automatically in the right places but completely transparent to the end user. Since **QB64-PE v4.3.0** we also use this auto-including feature for the new $USELIBRARY metacommand to ease the including of 3rd party libraries. As the entire process might not be 100% clear for everybody, it's detailed here for all QB64-PE developers.

#### In general the auto-include logic knows about three general include positions:
1. **AtTop** - This auto-including happens right at the beginning of the program as the compiler will see it, but is actually done before the first line the user has written in his program i.e. what you see as first line in the IDE respectively. In this place only initializing stuff CONST, (RE)DIM [SHARED], TYPE etc. can be done just like in an `*.bi` file of a library. The auto-include logic in the QB64-PE sources uses the **firstLine** variable to trigger and control including at this position.
2. **AfterMain** - This auto-including happens right before the first SUB or FUNCTION definition appears in the code, regardless if it appears in the main program file or inside of an $INCLUDE'd file. If no SUB/FUNCTION is used in the program at all, then auto-including happens after the last program line but still before the **AtBottom** auto-includes. In this place only main scope (global) DATAs, GOSUB routines and ERROR handlers can be done. If libraries use this, then respective include files should simply use the `*bas` extension just like the main program file. The auto-include logic in the QB64-PE sources uses the **mainEndLine** variable to trigger and control including at this position.
3. **AtBottom** - This auto-including happens right at the end of the program as the compiler will see it, but is actually done after the last line the user has written in his program i.e. what you see as last line in the IDE respectively. In this place only SUB and FUNCTION definitions can be done just like in an `*.bm` file of a library. The auto-include logic in the QB64-PE sources uses the **lastLine** variable to trigger and control including at this position.

The mentioned control variables work all the same and can have four different states during each compile pass: 0=inactive, 1=triggered, 2=in progress and 3=done. These states are used in various places in the compiler to control the flow and generate errors where necessary.

Obviously the **AtTop** and **AfterMain** files itself can not contain any SUB or FUNCTION definitions, because that would lead to interleaved main vs. SUB/FUNCTION code if more than one file is auto-included like when multiple libraries are pulled in for instance. A new compiler error was implemented to catch such cases. However, the auto-included files could of course pull in further $INCLUDEs, but those must also follow the conditions of the current auto-include level.

Further, if library **ABC** has dependencies on another library **XYZ**, then **XYZ** can be easily pulled in by placing the respective $USELIBRARY call inside the **AtTop** file of of the current library **ABC**. Each found $USELIBRARY will trigger a recompile to immediately allow the auto-include logic to adapt the resulting program structure.

The order how **AtTop** library files are included is reversed to the order as their respective $USELIBRARY lines appear in the code to make sure dependencies are pulled in before the library which actually depends on it. The **AfterMain** and **AtBottom** library files are just included in the order of their appearance.

#### The following figure details the resulting program structure as it is sent through the compiler inclusive all auto-include features:
    +---------------------------------------------------+
    | beforefirstline.bi (always, except the file is in |---> from internal/support/include
    | the IDE for editing (general CONST values))       |
    +---------------------------------------------------+
    | color0.bi or color32.bi (if $COLOR:0/32 is used   |---> from internal/support/color
    | and none of both is in the IDE for editing)       |
    \---------------------------------------------------/
     >> from here all general/color CONST can be used <<
    /---------------------------------------------------\
    | "AtTop" library files (if $USELIBRARY is used)    |---> from libraries/includes/<author>/<libname>
    | The order of libraries is reversed to the order of|
    | their appearance in code to serve dependencies.   |
    +---------------------------------------------------+
    | vwatch.bi (if $DEBUG is used or vwatch.bm is in   |---> from internal/support/vwatch
    | the IDE for editing)                              |
    +---------------------------------------------------+---  ---  ---  ---  ---  ---  ---  ---  ---  ---  --- --- ---
    | This is the user's main program currently in the  |          /--------------------------------------------------+
    | IDE or passed to the command line compiler.       |         / aftermain.bas (always, inject an implicit END)    |
    |      CONST, DIM, TYPE etc.                        |   +----<  The include happens right before the first SUB or |
    |      code...                                      |   |     \ FUNCTION, i.e. in the last line, if the main code |
    |      END                                          |   |      \ doesn't define any SUBs or FUNCTIONs.            |--+
    |      labelMyData:                                 |   |       \-----------+-------------------------------------+  |
    |      DATA ... ... ... ... ...                     |   |                   |   from internal/support/include <------+
    |      DATA ... ... ... ... ...                     |   |                   |
    |      labelMyGosub:                                |   |      /------------+-------------------------------------+
    |      code...                                      |   |     / "AfterMain" library files (if $USELIBRARY is used)|
    |      RETURN                                       |   |  +-<  The order of libraries is as they appear in code. |-----------+
    |      labelMyErrorHandler:                         |   |  |  \-------------+-------------------------------------+           |
    |      code...                                      |   |  |                |   from libraries/includes/<author>/<libname> <--+
    |      RESUME NEXT                                  |   |  |                ^
    |       ===============     <<<-------------------------+  |              NOTE:
    |       ===============     <<<----------------------------+    Works also if the main end (i.e. the 1st SUB/FUNC)
    |      SUB/FUNCTION MySubFunc                       |           is inside of a included file. Then the include
    |      code...                                      |           file is regularly continued (with 1st S/F) after
    |      END SUB/FUNCTION                             |           all "AfterMain" files are auto-included.
    +---------------------------------------------------+---  ---  ---  ---  ---  ---  ---  ---  ---  ---  --- --- ---
    | vwatch.bm (if $DEBUG is used, else vwatch_stub.bm)|
    | Only if none of the vwatch*.* files is in the IDE.|---> from internal/support/vwatch
    +---------------------------------------------------+
    | "AtBottom" library files (if $USELIBRARY is used) |
    | The order of libraries is as they appear in code. |---> from libraries/includes/<author>/<libname>
    +---------------------------------------------------+
    | afterlastline.bm (always, except the file is in   |
    | the IDE for editing (QB64-PE support functions))  |---> from internal/support/include
    +---------------------------------------------------+
