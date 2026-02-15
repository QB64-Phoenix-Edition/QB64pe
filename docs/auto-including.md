# The QB64-PE auto-include logic
We have a nice auto-including feature since **QB64-PE v4.0.0**, which makes sure that "under the hood" support files (general and color CONSTs, vwatch code ($DEBUG) and other support code) are included automatically in the right places but completely transparent to the end user. Since **QB64-PE v4.3.0** we also use this auto-including feature for the new $USELIBRARY metacommand to ease the including of 3rd party libraries. As the entire process might not be 100% clear for everybody, it's detailed here for all QB64-PE developers.

#### Traditionally the auto-include logic knows about three general include positions:
Long time QB64(PE) wasn't able to interleave main level code and SUB/FUNCTION code, hence there were clear rules where in the program flow certain things had to be placed, regardless if it was written in one big source file or included from smaller source units.

1. **AtTop** - This auto-including happens right at the beginning of the program as the compiler will see it, but is actually done before the first line of user code, i.e. what you see as first line in the IDE respectively. In this place only initializing stuff CONST, (RE)DIM [SHARED], TYPE etc. was allowed and files included here usually had the `*.bi` extension. The auto-include logic in the QB64-PE sources uses the **firstLine** variable to trigger and control including at this position.
2. **AfterMain** - This auto-including happens at the end of the main level code, the place where we usually put DATAs, GOSUB routines or ERROR handlers and files included here usually had the `*.bas` extension. Traditionally this include position was triggered right before the first SUB or FUNCTION definition appeared in the code, regardless if it was in the main program file or inside of an included file. If no SUB/FUNCTION was used in the program at all, then auto-including happened after the last program line but still before the **AtBottom** auto-includes. The auto-include logic in the QB64-PE sources uses the **mainEndLine** variable to trigger and control including at this position.<br>
**Note:** Since **QB64-PE v4.4.0** we can interleave main level and SUB/FUNCTION code, as a result of this auto-including at this position now always happens right before the **AtBottom** auto-includes, just like in an SUB/FUNCTION-less program in former QB64-PE versions.
3. **AtBottom** - This auto-including happens right at the end of the program as the compiler will see it, but is actually done after the last line of user code, i.e. what you see as last line in the IDE respectively. In this place only SUB and FUNCTION definitions were allowed and files included here usually had the `*.bm` extension. The auto-include logic in the QB64-PE sources uses the **lastLine** variable to trigger and control including at this position.

The mentioned control variables work all the same and can have four different states during each compile pass: 0=inactive, 1=triggered, 2=in progress and 3=done. These states are used in various places in the compiler to control the flow where necessary.

Regarding the **QB64-PE Libraries Pack** and the related $USELIBRARY meta-command, any library can pull in further libraries it depends on. Each hit on $USELIBRARY will immediately trigger a recompile to allow the auto-include logic to adapt the resulting program structure. The order how **AtTop** library files are included is reversed to the order as their respective $USELIBRARY lines appear in the code to make sure dependencies are pulled in before the library which actually depends on it. The **AfterMain** and **AtBottom** library files are just included in the order of their appearance.

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
    |      CONST, DIM, TYPE etc.                        |   +----<  The include happens at the end of the main code.  |--+
    |      code...                                      |   |     \---------------------------------------------------+  |
    |      END                                          |   |                       from internal/support/include <------+
    |      labelMyData:                                 |   |
    |      DATA ... ... ... ... ...                     |   |
    |      DATA ... ... ... ... ...                     |   |
    |      labelMyGosub:                                |   |      /--------------------------------------------------+
    |      code...                                      |   |     / "AfterMain" library files (if $USELIBRARY is used)|
    |      RETURN                                       |   |  +-<  The order of libraries is as they appear in code. |--+
    |      labelMyErrorHandler:                         |   |  |  \---------------------------------------------------+  |
    |      code...                                      |   |  |           from libraries/includes/<author>/<libname> <--+
    |      RESUME NEXT                                  |   |  |
    |       ===============     <<<-------------------------+  |
    |       ===============     <<<----------------------------+
    |      SUB/FUNCTION MySubFunc                       |
    |      code...                                      |
    |      END SUB/FUNCTION                             |
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
