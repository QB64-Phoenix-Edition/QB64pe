SUB reginternal
    reginternalsubfunc = 1

    'special argument codes:
    '-----------------------
    '-1 Any numeric variable (will be made explicit by a C cast)
    '   Typically, these are used when multiple C functions exist

    '-2 Offset+Size(in bytes)
    '   Size is the largest safe memory block available from the offset
    '   used for: CALL INTERRUPT[X]

    '-3 Offset+Size(in bytes)
    '   Size is the largest safe memory block available from the offset
    '   *Like -2, but restrictions apply
    '   used for: GET/PUT(graphics)

    '-4 Offset+Size(in bytes)
    '   Size is the size of the element referenced
    '   used for: GET/PUT(file)

    '-5 Offset only
    '-6 Size only

    '-7 '_MEM' structure referring to the passed variable

    'special return codes:
    '---------------------
    ' none yet

    'stubs for internally handled commands:
    clearid
    id.n = "Asc": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Asc": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "End": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "LSet": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "RSet": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Mid": id.subfunc = 2: id.callname = "sub_stub": id.musthave = "$": regid
    clearid
    id.n = "Print": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Option": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Swap": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "System": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Write": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Read": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Close": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Reset": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Input": id.subfunc = 2: id.callname = "sub_stub": regid
    'stubs for unimplemented commands:
    clearid
    id.n = "TrOn": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "TrOff": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "List": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "Def": id.subfunc = 2: id.callname = "sub_stub": id.secondargcantbe = "SEG": regid
    clearid
    id.n = "IoCtl": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "IoCtl": id.subfunc = 1: id.callname = "func_stub": id.musthave = "$": id.args = 1: id.arg = MKL$(LONGTYPE - ISPOINTER): id.ret = STRINGTYPE - ISPOINTER: regid
    clearid
    id.n = "Fre": id.subfunc = 1: id.callname = "func_stub": id.args = 1: id.arg = MKL$(LONGTYPE - ISPOINTER): id.ret = LONGTYPE - ISPOINTER: regid
    clearid
    id.n = "SetMem": id.subfunc = 1: id.callname = "func_stub": id.args = 1: id.arg = MKL$(LONGTYPE - ISPOINTER): id.ret = LONGTYPE - ISPOINTER: regid
    clearid
    id.n = "FileAttr": id.subfunc = 1: id.callname = "func_stub": id.args = 2: id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER): id.ret = LONGTYPE - ISPOINTER: regid
    clearid
    id.n = "_MemGet": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "_MemPut": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "_MemFill": id.subfunc = 2: id.callname = "sub_stub": regid
    clearid
    id.n = "_Continue": id.subfunc = 2: id.callname = "sub_stub": regid

    clearid
    id.n = "_Cast"
    id.subfunc = 1
    id.args = 2
    id.arg = MKL$(-1) + MKL$(-1) ' overridden in qb64pe.bas
    id.ret = -1 ' overridden in qb64pe.bas
    id.hr_syntax = "_CAST(numericalType, numericalValue)"
    regid

    clearid
    id.n = "_IIf"
    id.subfunc = 1
    id.args = 3
    id.arg = MKL$(OFFSETTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) ' overridden in qb64pe.bas
    id.ret = STRINGTYPE - ISPOINTER ' overridden in qb64pe.bas
    id.hr_syntax = "_IIF(expression, truePart, falsePart)"
    regid

    clearid
    id.n = "_Resize"
    id.subfunc = 2
    id.callname = "sub__resize"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{On|Off}][,{_Stretch|_Smooth}]"
    id.hr_syntax = "_RESIZE [{ON|OFF}][, {_STRETCH|_SMOOTH}]"
    regid

    clearid
    id.n = "_Resize"
    id.subfunc = 1
    id.callname = "func__resize"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_RESIZE"
    regid

    clearid
    id.n = "_ResizeWidth"
    id.subfunc = 1
    id.callname = "func__resizewidth"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_RESIZEWIDTH"
    regid

    clearid
    id.n = "_ResizeHeight"
    id.subfunc = 1
    id.callname = "func__resizeheight"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_RESIZEHEIGHT"
    regid

    clearid
    id.n = "_ScaledWidth"
    id.subfunc = 1
    id.callname = "func__scaledwidth"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCALEDWIDTH"
    regid

    clearid
    id.n = "_ScaledHeight"
    id.subfunc = 1
    id.callname = "func__scaledheight"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCALEDHEIGHT"
    regid


    clearid
    id.n = "_GLRender"
    id.subfunc = 2
    id.callname = "sub__glrender"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{_Behind|_OnTop|_Only}"
    id.hr_syntax = "_GLRENDER {_Behind|_OnTop|_Only}"
    regid

    clearid
    id.n = "_DisplayOrder"
    id.subfunc = 2
    id.callname = "sub__displayorder"
    id.args = 4
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{_Software|_Hardware|_Hardware1|_GLRender}[,{_Software|_Hardware|_Hardware1|_GLRender}[,{_Software|_Hardware|_Hardware1|_GLRender}[,{_Software|_Hardware|_Hardware1|_GLRender}]]]]"
    id.hr_syntax = "_DISPLAYORDER {_Software|_Hardware|_Hardware1|_GLRender} (any combination/order)"
    regid

    clearid
    id.n = "_MemGet"
    id.subfunc = 1
    id.callname = "func__memget"
    id.args = 3
    id.arg = MKL$(UDTTYPE + (1)) + MKL$(OFFSETTYPE - ISPOINTER) + MKL$(-1) 'x = _MEMGET(block, offset, type)
    id.ret = -1
    id.hr_syntax = "_MEMGET(block, offset, type)"
    regid

    clearid
    id.n = "_Mem"
    id.subfunc = 1
    id.callname = "func__mem"
    'id.args = 1
    'id.arg = MKL$(-7)
    id.args = 2
    id.arg = MKL$(OFFSETTYPE - ISPOINTER) + MKL$(OFFSETTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = ISUDT + (1) 'the _MEM type is the first TYPE defined
    id.hr_syntax = "_MEM(referenceVariable) or _MEM(offset, byteSize)"
    regid
    '---special case---

    clearid
    id.n = "_MemElement"
    id.subfunc = 1
    id.callname = "func__mem"
    id.args = 1
    id.arg = MKL$(-8)
    id.ret = ISUDT + (1) 'the _MEM type is the first TYPE defined
    id.hr_syntax = "_MEMELEMENT(referenceVariable)"
    regid
    '---special case---

    clearid
    id.n = "_MemFree"
    id.subfunc = 2
    id.callname = "sub__memfree"
    id.args = 1
    id.arg = MKL$(UDTTYPE + (1))
    id.hr_syntax = "_MEMFREE memoryVariable"
    regid

    clearid
    id.n = "_MemExists"
    id.subfunc = 1
    id.callname = "func__memexists"
    id.args = 1
    id.arg = MKL$(UDTTYPE + (1))
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_MEMEXISTS(memBlock)"
    regid

    clearid
    id.n = "_MemNew"
    id.subfunc = 1
    id.callname = "func__memnew"
    id.args = 1
    id.arg = MKL$(OFFSETTYPE - ISPOINTER)
    id.ret = ISUDT + (1) 'the _MEM type is the first TYPE defined
    id.hr_syntax = "_MEMNEW(byteSize)"
    regid

    clearid
    id.n = "_MemImage"
    id.subfunc = 1
    id.callname = "func__memimage"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]" 'dest is default
    id.ret = ISUDT + (1) 'the _MEM type is the first TYPE defined
    id.hr_syntax = "_MEMIMAGE or _MEMIMAGE(imageHandle)"
    regid

    clearid
    id.n = "_MemSound": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__memsound"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = ISUDT + (1) 'the _MEM type is the first TYPE defined
    id.hr_syntax = "_MEMSOUND(soundHandle)"
    regid

    clearid '_MEMCOPY a, aoffset, bytes TO b, boffset
    id.n = "_MemCopy"
    id.subfunc = 2
    id.callname = "sub__memcopy"
    id.args = 5
    id.arg = MKL$(UDTTYPE + (1)) + MKL$(OFFSETTYPE - ISPOINTER) + MKL$(OFFSETTYPE - ISPOINTER) + MKL$(UDTTYPE + (1)) + MKL$(OFFSETTYPE - ISPOINTER)
    id.specialformat = "?,?,?{To}?,?" 'dest is default
    id.hr_syntax = "_MEMCOPY sourceBlock, sourceBlock.OFFSET, sourceBlock.SIZE TO destBlock, destBlock.OFFSET"
    regid

    clearid
    id.n = "_ConsoleTitle"
    id.subfunc = 2
    id.callname = "sub__consoletitle"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_CONSOLETITLE title$"
    regid

    clearid
    id.n = "_ScreenShow"
    id.subfunc = 2
    id.callname = "sub__screenshow"
    id.hr_syntax = "_SCREENSHOW"
    regid

    clearid
    id.n = "_ScreenHide"
    id.subfunc = 2
    id.callname = "sub__screenhide"
    id.hr_syntax = "_SCREENHIDE"
    regid

    clearid
    id.n = "_ScreenHide"
    id.subfunc = 1
    id.callname = "func__screenhide"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCREENHIDE"
    regid

    clearid
    id.n = "_Console"
    id.subfunc = 1
    id.callname = "func__console"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CONSOLE"
    regid

    clearid
    id.n = "_Console"
    id.subfunc = 2
    id.callname = "sub__console"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{On|Off}"
    id.hr_syntax = "_CONSOLE {On|Off}"
    regid

    clearid
    id.n = "_ControlChr"
    id.subfunc = 2
    id.callname = "sub__controlchr"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{On|Off}"
    id.hr_syntax = "_CONTROLCHR {On|Off}"
    regid

    clearid
    id.n = "_Blink"
    id.subfunc = 2
    id.callname = "sub__blink"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{On|Off}"
    id.hr_syntax = "_BLINK {On|Off}"
    regid

    clearid
    id.n = "_Blink"
    id.subfunc = 1
    id.callname = "func__blink"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_BLINK"
    regid

    clearid
    id.n = "_FileExists"
    id.subfunc = 1
    id.callname = "func__fileexists"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_FILEEXISTS(fileName$)"
    regid

    clearid
    id.n = "_DirExists"
    id.subfunc = 1
    id.callname = "func__direxists"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_DIREXISTS(path$)"
    regid

    'QB64 DEVICE interface

    clearid
    id.n = "Stick": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func_stick"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "?[,?]"
    id.hr_syntax = "STICK(direction%) or STICK(direction%, axis_number%)"
    regid

    clearid
    id.n = "Strig": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func_strig"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "?[,?]"
    id.hr_syntax = "STRIG(button_function%) or STRIG(button_function%, device_number%)"
    regid

    clearid
    id.n = "Strig": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 2
    id.callname = "sub_strig"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[(?[,?])]{On|Off|Stop}"
    'In previous versions of BASIC, the statement STRIG ON enables testing of the joystick triggers; STRIG OFF disables joystick trigger testing. QuickBASIC ignores STRIG ON and STRIG OFF statements--the statements are provided for compatibility with earlier versions.
    id.hr_syntax = "STRIG(button%) {On|Off|Stop}"
    regid

    clearid
    id.n = "_Devices": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__devices"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_DEVICES"
    regid

    clearid
    id.n = "_Device": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__device"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_DEVICE$(device_number)"
    regid

    clearid
    id.n = "_DeviceInput": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__deviceinput"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_DEVICEINPUT or _DEVICEINPUT(device_number%)"
    regid

    clearid
    id.n = "_LastButton": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__lastbutton"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_LASTBUTTON(deviceNumber)"
    regid

    clearid
    id.n = "_LastAxis": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__lastaxis"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_LASTAXIS(deviceNumber)"
    regid

    clearid
    id.n = "_LastWheel": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__lastwheel"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_LASTWHEEL(deviceNumber)"
    regid

    clearid
    id.n = "_Button": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__button"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_BUTTON(button_number%)"
    regid

    clearid
    id.n = "_ButtonChange": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__buttonchange"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_BUTTONCHANGE(button_number%)"
    regid

    clearid
    id.n = "_Axis": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__axis"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = SINGLETYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_AXIS(axis_number%)"
    regid

    clearid
    id.n = "_Wheel": id.Dependency = DEPENDENCY_DEVICEINPUT
    id.subfunc = 1
    id.callname = "func__wheel"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = SINGLETYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_WHEEL(wheelNumber%)"
    regid

    clearid
    id.n = "Key"
    id.subfunc = 2
    id.callname = "sub_key"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "(?){On|Off|Stop}"
    id.hr_syntax = "KEY(number) {On|Off|Stop}"
    regid

    clearid
    id.n = "_ScreenX"
    id.subfunc = 1
    id.callname = "func__screenx"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCREENX"
    regid

    clearid
    id.n = "_ScreenY"
    id.subfunc = 1
    id.callname = "func__screeny"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCREENY"
    regid

    clearid
    id.n = "_ScreenMove"
    id.subfunc = 2
    id.callname = "sub__screenmove"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{_Middle}][?,?]"
    id.hr_syntax = "_SCREENMOVE x, y or _SCREENMOVE _Middle"
    regid

    clearid
    id.n = "_MouseMove"
    id.subfunc = 2
    id.callname = "sub__mousemove"
    id.args = 2
    id.arg = MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER)
    id.hr_syntax = "_MOUSEMOVE x, y"
    regid

    clearid
    id.n = "_OS"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__os"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_OS$"
    regid

    clearid
    id.n = "_Title"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__title"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_TITLE$"
    regid

    clearid
    id.n = "_MapUnicode"
    id.subfunc = 2
    id.callname = "sub__mapunicode"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?{To}?"
    id.hr_syntax = "_MAPUNICODE unicode& TO asciiCode%"
    regid

    clearid
    id.n = "_MapUnicode"
    id.subfunc = 1
    id.callname = "func__mapunicode"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_MAPUNICODE(asciiCode%)"
    regid

    clearid
    id.n = "_KeyDown"
    id.subfunc = 1
    id.callname = "func__keydown"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_KEYDOWN(code&)"
    regid

    clearid
    id.n = "_KeyHit"
    id.subfunc = 1
    id.callname = "func__keyhit"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_KEYHIT"
    regid

    clearid
    id.n = "_WindowHandle"
    id.subfunc = 1
    id.callname = "func__handle"
    id.ret = INTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_WINDOWHANDLE"
    regid

    clearid
    id.n = "Files"
    id.subfunc = 2
    id.callname = "sub_files"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "FILES fileSpec$"
    regid

    clearid
    id.n = "_Files"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__files"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_FILES$([fileSpec$])"
    regid

    clearid
    id.n = "_FullPath"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__fullpath"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_FULLPATH$(pathName$)"
    regid

    clearid
    id.n = "_PrintImage": id.Dependency = DEPENDENCY_PRINTER
    id.subfunc = 2
    id.callname = "sub__printimage"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    ''proposed version:
    ''id.specialformat = "[_SQUAREPIXELS][?][,(?,?)-(?,?)]"
    id.hr_syntax = "_PRINTIMAGE imageHandle&"
    regid

    'remote desktop

    clearid
    id.n = "_ScreenClick"
    id.subfunc = 2
    id.callname = "sub__screenclick"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?[,?]"
    id.hr_syntax = "_SCREENCLICK x, y[, button%]"
    regid

    clearid
    id.n = "_ScreenPrint"
    id.subfunc = 2
    id.callname = "sub__screenprint"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_SCREENPRINT text$"
    regid

    clearid
    id.n = "_ScreenImage": id.Dependency = DEPENDENCY_SCREENIMAGE
    id.subfunc = 1
    id.callname = "func__screenimage"
    id.args = 4
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?,?,?,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCREENIMAGE(column1, row1, column2, row2)"
    regid

    clearid
    id.n = "Lock"
    id.subfunc = 2
    id.callname = "sub_lock"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(INTEGER64TYPE - ISPOINTER) + MKL$(INTEGER64TYPE - ISPOINTER)
    id.specialformat = "[#]?[,[?][{To}?]]"
    id.hr_syntax = "LOCK #fileNumber%, record& or LOCK #fileNumber% firstRecord& TO lastRecord&"
    regid

    clearid
    id.n = "Unlock"
    id.subfunc = 2
    id.callname = "sub_unlock"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(INTEGER64TYPE - ISPOINTER) + MKL$(INTEGER64TYPE - ISPOINTER)
    id.specialformat = "[#]?[,[?][{To}?]]"
    id.hr_syntax = "UNLOCK #fileNumber%, record& or UNLOCK #fileNumber% firstRecord& TO lastRecord&"
    regid

    clearid
    id.n = "_FreeTimer"
    id.subfunc = 1
    id.callname = "func__freetimer"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_FREETIMER"
    regid

    clearid
    id.n = "Timer"
    id.subfunc = 2
    id.callname = "sub_timer"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[(?)]{On|Off|Stop|Free}"
    id.hr_syntax = "TIMER[(number%)] {On|Off|Stop|Free}"
    regid

    clearid
    id.n = "_FullScreen"
    id.subfunc = 2
    id.callname = "sub__fullscreen"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{_Off|_Stretch|_SquarePixels|Off}][,{_Smooth}]"
    id.hr_syntax = "_FULLSCREEN [{_Off|_Stretch|_SquarePixels|Off}][,{_Smooth}]"
    regid

    clearid
    id.n = "_AllowFullScreen"
    id.subfunc = 2
    id.callname = "sub__allowfullscreen"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{_Stretch|_SquarePixels|_Off|_All|Off}][,{_Smooth|_Off|_All|Off}]"
    id.hr_syntax = "_ALLOWFULLSCREEN [{_Stretch|_SquarePixels|_Off|_All|Off}][,{_Smooth|_Off|_All|Off}]"
    regid

    clearid
    id.n = "_FullScreen"
    id.subfunc = 1
    id.callname = "func__fullscreen"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_FULLSCREEN"
    regid

    clearid
    id.n = "_Smooth"
    id.subfunc = 1
    id.callname = "func__fullscreensmooth"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SMOOTH"
    regid

    clearid
    id.n = "_WindowHasFocus"
    id.subfunc = 1
    id.callname = "func__hasfocus"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_WINDOWHASFOCUS"
    regid

    clearid
    id.n = "_Clipboard"
    id.musthave = "$"
    id.subfunc = 2
    id.callname = "sub__clipboard"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "=?"
    id.hr_syntax = "_CLIPBOARD$ = text$"
    regid

    clearid
    id.n = "_Clipboard"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__clipboard"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_CLIPBOARD$"
    regid

    clearid
    id.n = "_ClipboardImage": id.Dependency = DEPENDENCY_SCREENIMAGE
    id.subfunc = 1
    id.callname = "func__clipboardimage"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CLIPBOARDIMAGE"
    regid

    clearid
    id.n = "_ClipboardImage": id.Dependency = DEPENDENCY_SCREENIMAGE
    id.subfunc = 2
    id.callname = "sub__clipboardimage"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "=?"
    id.hr_syntax = "_CLIPBOARDIMAGE = existingImageHandle&"
    regid

    clearid
    id.n = "_Exit"
    id.subfunc = 1
    id.callname = "func__exit"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_EXIT"
    regid

    clearid
    id.n = "_OpenHost": id.Dependency = DEPENDENCY_SOCKETS
    id.subfunc = 1
    id.callname = "func__openhost"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_OPENHOST(" + CHR$(34) + "TCP/IP:portNumber" + CHR$(34) + ")"
    regid

    clearid
    id.n = "_Connected"
    id.subfunc = 1
    id.callname = "func__connected"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CONNECTED(connectionHandle&)"
    regid

    clearid
    id.n = "_ConnectionAddress"
    id.mayhave = "$"
    id.subfunc = 1
    id.callname = "func__connectionaddress"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_CONNECTIONADDRESS(connectionHandle&)"
    regid

    clearid
    id.n = "_OpenConnection"
    id.subfunc = 1
    id.callname = "func__openconnection"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_OPENCONNECTION(hostHandle)"
    regid

    clearid
    id.n = "_OpenClient": id.Dependency = DEPENDENCY_SOCKETS
    id.subfunc = 1
    id.callname = "func__openclient"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_OPENCLIENT(" + CHR$(34) + "TCP/IP:port:address" + CHR$(34) + ")"
    regid

    clearid
    id.n = "_StatusCode"
    id.subfunc = 1
    id.callname = "func__statusCode"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_STATUSCODE(httpHandle&)"
    regid

    clearid
    id.n = "_EnvironCount"
    id.subfunc = 1
    id.callname = "func__environcount"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_ENVIRONCOUNT"
    regid

    clearid
    id.n = "Environ"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_environ"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "ENVIRON$(listIndex%) or ENVIRON$(systemID$)"
    regid

    clearid
    id.n = "Environ"
    id.subfunc = 2
    id.callname = "sub_environ"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "ENVIRON stringExpression$"
    regid

    clearid
    id.n = "_ErrorLine"
    id.subfunc = 1
    id.callname = "func__errorline"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_ERRORLINE"
    regid

    clearid
    id.n = "_InclErrorLine"
    id.subfunc = 1
    id.callname = "func__inclerrorline"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_INCLERRORLINE"
    regid

    clearid
    id.n = "_ErrorMessage"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__errormessage"
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.args = 1
    id.specialformat = "[?]"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_ERRORMESSAGE$ or _ERRORMESSAGE$(errorCode%)"
    regid

    clearid
    id.n = "_Assert"
    id.subfunc = 2
    id.callname = "sub__assert"
    id.args = 2
    id.specialformat = "?[,?]"
    id.arg = MKL$(INTEGERTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_ASSERT condition[, errorMessage$]"
    regid

    clearid
    id.n = "_Display"
    id.subfunc = 2
    id.callname = "sub__display"
    id.hr_syntax = "_DISPLAY"
    regid

    clearid
    id.n = "_AutoDisplay"
    id.subfunc = 2
    id.callname = "sub__autodisplay"
    id.hr_syntax = "_AUTODISPLAY"
    regid

    clearid
    id.n = "_Limit"
    id.subfunc = 2
    id.callname = "sub__limit"
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.hr_syntax = "_LIMIT framesPerSecond!"
    regid

    clearid
    id.n = "_FPS"
    id.subfunc = 2
    id.callname = "sub__fps"
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.specialformat = "[{_Auto}][?]"
    id.hr_syntax = "_FPS fps! or _FPS _Auto"
    regid

    clearid
    id.n = "_Delay"
    id.subfunc = 2
    id.callname = "sub__delay"
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.hr_syntax = "_DELAY seconds!"
    regid

    clearid
    id.n = "_Icon": id.Dependency = DEPENDENCY_ICON
    id.subfunc = 2
    id.callname = "sub__icon"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?[,?]]"
    id.hr_syntax = "_ICON [mainImageHandle&[, smallImageHandle&]]"
    regid

    clearid
    id.n = "_Title"
    id.subfunc = 2
    id.callname = "sub__title"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_TITLE text$"
    regid

    clearid
    id.n = "_Echo"
    id.subfunc = 2
    id.callname = "sub__echo"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_ECHO text$"
    regid

    clearid
    id.n = "_ReadFile"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__readfile"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_READFILE$(fileSpec$)"
    regid

    clearid
    id.n = "_WriteFile"
    id.subfunc = 2
    id.callname = "sub__writefile"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_WRITEFILE fileSpec$, contents$"
    regid

    clearid
    id.n = "_AcceptFileDrop"
    id.subfunc = 2
    id.callname = "sub__filedrop"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{On|Off}]"
    id.hr_syntax = "_ACCEPTFILEDROP [{On|Off}]"
    regid

    clearid
    id.n = "_AcceptFileDrop"
    id.subfunc = 1
    id.callname = "func__filedrop"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_ACCEPTFILEDROP"
    regid

    clearid
    id.n = "_FinishDrop"
    id.subfunc = 2
    id.callname = "sub__finishdrop"
    id.hr_syntax = "_FINISHDROP"
    regid

    clearid
    id.n = "_TotalDroppedFiles"
    id.subfunc = 1
    id.callname = "func__totaldroppedfiles"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_TOTALDROPPEDFILES"
    regid

    clearid
    id.n = "_DroppedFile"
    id.mayhave = "$"
    id.subfunc = 1
    id.callname = "func__droppedfile"
    id.ret = STRINGTYPE - ISPOINTER
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "_DROPPEDFILE$ or _DROPPEDFILE$(index&)"
    regid

    clearid
    id.n = "Clear"
    id.subfunc = 2
    id.callname = "sub_clear"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,?]]"
    id.hr_syntax = "CLEAR"
    regid

    'IMAGE CREATION/FREEING

    clearid
    id.n = "_NewImage"
    id.subfunc = 1
    id.callname = "func__newimage"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_NEWIMAGE(width&, height&, mode)"
    regid

    clearid
    id.n = "_LoadImage"
    id.Dependency = DEPENDENCY_IMAGE_CODEC
    id.subfunc = 1
    id.callname = "func__loadimage"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "?[,[?][,?]]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_LOADIMAGE(fileName$[, [mode&][, requirements$]])"
    regid

    clearid
    id.n = "_FreeImage"
    id.subfunc = 2
    id.callname = "sub__freeimage"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "_FREEIMAGE handle&"
    regid

    clearid
    id.n = "_CopyImage"
    id.subfunc = 1
    id.callname = "func__copyimage"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_COPYIMAGE(imageHandle&[, mode])"
    regid

    clearid
    id.n = "_SaveImage"
    id.Dependency = DEPENDENCY_IMAGE_CODEC
    id.subfunc = 2
    id.callname = "sub__saveimage"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "?[,[?][,?]]"
    id.hr_syntax = "_SAVEIMAGE fileName$[, imageHandle&][, requirements$])"
    regid

    'IMAGE SELECTION

    clearid
    id.n = "_Source"
    id.subfunc = 2
    id.callname = "sub__source"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?"
    id.hr_syntax = "_SOURCE handle&"
    regid

    clearid
    id.n = "_Dest"
    id.subfunc = 2
    id.callname = "sub__dest"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?"
    id.hr_syntax = "_DEST imageHandle&"
    regid

    clearid
    id.n = "_Source"
    id.subfunc = 1
    id.callname = "func__source"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SOURCE"
    regid

    clearid
    id.n = "_Dest"
    id.subfunc = 1
    id.callname = "func__dest"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_DEST"
    regid

    clearid
    id.n = "_Display"
    id.subfunc = 1
    id.callname = "func__display"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_DISPLAY"
    regid

    'IMAGE SETTINGS

    clearid
    id.n = "_Blend"
    id.subfunc = 2
    id.callname = "sub__blend"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "_BLEND [imageHandle&]"
    regid

    clearid
    id.n = "_DontBlend"
    id.subfunc = 2
    id.callname = "sub__dontblend"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "_DONTBLEND [imageHandle&]"
    regid

    clearid
    id.n = "_ClearColor"
    id.subfunc = 2
    id.callname = "sub__clearcolor"
    id.args = 2
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{_None}][?][,?]"
    id.hr_syntax = "_CLEARCOLOR {color&|_None}[,dest_handle&]"
    regid

    'USING/CHANGING A SURFACE

    clearid
    id.n = "_PutImage"
    id.subfunc = 2
    id.callname = "sub__putimage"
    id.args = 10
    id.arg = MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER)
    id.specialformat = "[[{Step}](?,?)[-[{Step}](?,?)]][,[?][,[?][,[[{Step}](?,?)[-[{Step}](?,?)]][,{_Smooth}]]]]"
    id.hr_syntax = "_PUTIMAGE [STEP] [(dx1, dy1)-[STEP][(dx2, dy2)]][, sourceHandle&][, destHandle&][, ][STEP][(sx1, sy1)[-STEP][(sx2, sy2)]][_SMOOTH]"
    regid

    clearid
    id.n = "_MapTriangle"
    id.subfunc = 2
    id.callname = "sub__maptriangle"
    id.args = 19
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{_Clockwise|_AntiClockwise}][{_Seamless}](?,?)-(?,?)-(?,?)[,?]{To}(?,?[,?])-(?,?[,?])-(?,?[,?])[,[?][,{_Smooth|_SmoothShrunk|_SmoothStretched}]]"
    id.hr_syntax = "_MAPTRIANGLE [{_SEAMLESS}] (sx1, sy1)-(sx2, sy2)-(sx3, sy3), source& TO (dx1, dy1)-(dx2, dy2)-(dx3, dy3)[, destination&][{_SMOOTH|_SMOOTHSHRUNK|_SMOOTHSTRETCHED}]]"
    regid

    clearid
    id.n = "_DepthBuffer"
    id.subfunc = 2
    id.callname = "sub__depthbuffer"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{On|Off|Lock|_Clear}[,?]"
    id.hr_syntax = "_DEPTHBUFFER {On|Off|Lock|_Clear}[,handle&]"
    regid

    clearid
    id.n = "_SetAlpha"
    id.subfunc = 2
    id.callname = "sub__setalpha"
    id.args = 4
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,[?[{To}?]][,?]]"
    id.hr_syntax = "_SETALPHA alpha& or _SETALPHA color1& TO color2&[, imageHandle&]"
    regid

    'IMAGE INFO

    clearid
    id.n = "_Width"
    id.subfunc = 1
    id.callname = "func__width"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_WIDTH(imageHandle&)"
    regid

    clearid
    id.n = "_Height"
    id.subfunc = 1
    id.callname = "func__height"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_HEIGHT(imageHandle&)"
    regid

    clearid
    id.n = "_PixelSize"
    id.subfunc = 1
    id.callname = "func__pixelsize"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_PIXELSIZE[(imageHandle&)]"
    regid

    clearid
    id.n = "_ClearColor"
    id.subfunc = 1
    id.callname = "func__clearcolor"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CLEARCOLOR[(sourceHandle&)]"
    regid

    clearid
    id.n = "_Blend"
    id.subfunc = 1
    id.callname = "func__blend"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_BLEND[(imageHandle&)]"
    regid

    clearid
    id.n = "_DefaultColor"
    id.subfunc = 1
    id.callname = "func__defaultcolor"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_DEFAULTCOLOR[(imageHandle&)]"
    regid

    clearid
    id.n = "_BackgroundColor"
    id.subfunc = 1
    id.callname = "func__backgroundcolor"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_BACKGROUNDCOLOR(imageHandle&)]"
    regid

    '256 COLOR PALETTES

    clearid
    id.n = "_PaletteColor"
    id.subfunc = 1
    id.callname = "func__palettecolor"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_PALETTECOLOR(attributeNumber%, imgHandle&)"
    regid

    clearid
    id.n = "_PaletteColor"
    id.subfunc = 2
    id.callname = "sub__palettecolor"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?[,?]"
    id.hr_syntax = "_PALETTECOLOR attribute%, newColor&[, imgHandle&]"
    regid

    clearid
    id.n = "_CopyPalette"
    id.subfunc = 2
    id.callname = "sub__copypalette"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?][,?]"
    id.hr_syntax = "_COPYPALETTE [sourceImageHandle&[, destinationImageHandle&]]"
    regid

    'FONT SUPPORT

    clearid
    id.n = "_LoadFont": id.Dependency = DEPENDENCY_LOADFONT
    id.subfunc = 1
    id.callname = "func__loadfont"
    id.args = 4
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?[,[?][,[?]]]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_LOADFONT(fileName$, size&[, " + CHR$(34) + "{MONOSPACE|, UNICODE|, DONTBLEND|, MEMORY|, AUTOMONO}" + CHR$(34) + "][, fontIndex])"
    regid

    clearid
    id.n = "_Font"
    id.subfunc = 2
    id.callname = "sub__font"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.hr_syntax = "_FONT fontHandle&[, imageHandle&]"
    regid

    clearid
    id.n = "_FontWidth"
    id.subfunc = 1
    id.callname = "func__fontwidth"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_FONTWIDTH or _FONTWIDTH(fontHandle&)"
    regid

    clearid
    id.n = "_FontHeight"
    id.subfunc = 1
    id.callname = "func__fontheight"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_FONTHEIGHT or _FONTHEIGHT(fontHandle&)"
    regid

    clearid
    id.n = "_Font"
    id.subfunc = 1
    id.callname = "func__font"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_FONT[(imageHandle&)]"
    regid

    clearid
    id.n = "_PrintString"
    id.subfunc = 2
    id.callname = "sub__printstring"
    id.args = 4
    id.arg = MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?),?[,?]"
    id.hr_syntax = "_PRINTSTRING(x, y), text$[, imageHandle&]"
    regid

    clearid
    id.n = "_PrintWidth"
    id.subfunc = 1
    id.callname = "func__printwidth"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_PRINTWIDTH(textToPrint$[, destinationHandle&])"
    regid

    clearid
    id.n = "_FreeFont"
    id.subfunc = 2
    id.callname = "sub__freefont"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?"
    id.hr_syntax = "_FREEFONT fontHandle&"
    regid

    clearid
    id.n = "_PrintMode"
    id.subfunc = 2
    id.callname = "sub__printmode"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{_FillBackground|_KeepBackground|_OnlyBackground}[,?]"
    id.hr_syntax = "_PRINTMODE {_FillBackground|_KeepBackground|_OnlyBackground}[, imageHandle&]"
    regid

    clearid
    id.n = "_PrintMode"
    id.subfunc = 1
    id.callname = "func__printmode"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_PRINTMODE[(imageHandle&)]"
    regid

    clearid
    id.n = "_UFontHeight"
    id.Dependency = DEPENDENCY_LOADFONT
    id.subfunc = 1
    id.callname = "func__UFontHeight"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_UFONTHEIGHT&[(fontHandle&)]"
    regid

    clearid
    id.n = "_UPrintWidth"
    id.Dependency = DEPENDENCY_LOADFONT
    id.subfunc = 1
    id.callname = "func__UPrintWidth"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,[?][,?]]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_UPRINTWIDTH&(text$[, utfEncoding&][, fontHandle&])"
    regid

    clearid
    id.n = "_ULineSpacing"
    id.Dependency = DEPENDENCY_LOADFONT
    id.subfunc = 1
    id.callname = "func__ULineSpacing"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_ULINESPACING&[(fontHandle&)]"
    regid

    clearid
    id.n = "_UPrintString"
    id.Dependency = DEPENDENCY_LOADFONT
    id.subfunc = 2
    id.callname = "sub__UPrintString"
    id.args = 7
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "(?,?),?[,[?][,[?][,[?][,?]]]]"
    id.hr_syntax = "_UPRINTSTRING (x&, y&), text$[, maxWidth&][, utfEncoding&][, fontHandle&][, imageHandle&]"
    regid

    clearid
    id.n = "_UCharPos"
    id.Dependency = DEPENDENCY_LOADFONT
    id.subfunc = 1
    id.callname = "func__UCharPos"
    id.args = 4
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(-2) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) ' arg 2 has special handling code in evaluatefunc()
    id.specialformat = "?[,[?][,[?][,?]]]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_UCHARPOS&(text$[, posArray&([index&])][, utfEncoding&][, fontHandle&])"
    regid

    'WORKING WITH COLORS

    clearid
    id.n = "_RGBA"
    id.subfunc = 1
    id.callname = "func__rgba"
    id.args = 5
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?,?,?[,?]"
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_RGBA(red&, green&, blue&, alpha&[, imageHandle&])"
    regid

    clearid
    id.n = "_RGB"
    id.subfunc = 1
    id.callname = "func__rgb"
    id.args = 4
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?,?[,?]"
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_RGB(red&, green&, blue&[, imageHandle&])"
    regid

    clearid
    id.n = "_Red"
    id.subfunc = 1
    id.callname = "func__red"
    id.args = 2
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_RED(rgbaColorIndex&[, imageHandle&])"
    regid

    clearid
    id.n = "_Green"
    id.subfunc = 1
    id.callname = "func__green"
    id.args = 2
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_GREEN(rgbaColorIndex&[, imageHandle&])"
    regid

    clearid
    id.n = "_Blue"
    id.subfunc = 1
    id.callname = "func__blue"
    id.args = 2
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_BLUE(rgbaColorIndex&[, imageHandle&])"
    regid

    clearid
    id.n = "_Alpha"
    id.subfunc = 1
    id.callname = "func__alpha"
    id.args = 2
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_ALPHA(color~&[, imageHandle&])"
    regid

    clearid
    id.n = "_RGBA32"
    id.subfunc = 1
    id.callname = "func__rgba32"
    id.args = 4
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_RGBA32(red&, green&, blue&, alpha&)"
    regid

    clearid
    id.n = "_RGB32"
    id.subfunc = 1
    id.callname = "func__rgb32"
    id.overloaded = -1
    id.minargs = 1
    id.args = 4
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_RGB32(red&, green&, blue&[, alpha&]) or _RGB32(intensity&[, alpha&])"
    regid

    clearid
    id.n = "_Red32"
    id.subfunc = 1
    id.callname = "func__red32"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_RED32(rgbaColor&)"
    regid

    clearid
    id.n = "_Green32"
    id.subfunc = 1
    id.callname = "func__green32"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_GREEN32(rgbaColor&)"
    regid

    clearid
    id.n = "_Blue32"
    id.subfunc = 1
    id.callname = "func__blue32"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_BLUE32(rgbaColor&)"
    regid

    clearid
    id.n = "_Alpha32"
    id.subfunc = 1
    id.callname = "func__alpha32"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_ALPHA32(rgbaColor&)"
    regid

    clearid
    id.n = "_HSB32"
    id.subfunc = 1
    id.callname = "func__hsb32"
    id.args = 3
    id.arg = MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER)
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_HSB32(hue#, saturation#, brightness#)"
    regid

    clearid
    id.n = "_HSBA32"
    id.subfunc = 1
    id.callname = "func__hsba32"
    id.args = 4
    id.arg = MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER)
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_HSBA32(hue#, saturation#, brightness#, alpha#)"
    regid

    clearid
    id.n = "_Hue32"
    id.subfunc = 1
    id.callname = "func__hue32"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "_HUE32(rgbaColor&)"
    regid

    clearid
    id.n = "_Saturation32"
    id.subfunc = 1
    id.callname = "func__sat32"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "_SATURATION32(rgbaColor&)"
    regid

    clearid
    id.n = "_Brightness32"
    id.subfunc = 1
    id.callname = "func__bri32"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "_BRIGHTNESS32(rgbaColor&)"
    regid

    clearid
    id.n = "Draw"
    id.subfunc = 2
    id.callname = "sub_draw"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "DRAW drawString$"
    regid

    'QB64 MOUSE

    clearid
    id.n = "_MouseShow"
    id.subfunc = 2
    id.callname = "sub__mouseshow"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "_MOUSESHOW [cursorShape$]"
    regid

    clearid
    id.n = "_MouseHide"
    id.subfunc = 2
    id.callname = "sub__mousehide"
    id.hr_syntax = "_MOUSEHIDE"
    regid

    clearid
    id.n = "_MouseHidden"
    id.subfunc = 1
    id.callname = "func__mousehidden"
    id.ret = LONGTYPE - ISPOINTER
    id.args = 0
    id.hr_syntax = "_MOUSEHIDDEN"
    regid

    clearid
    id.n = "_MouseInput"
    id.subfunc = 1
    id.callname = "func__mouseinput"
    id.ret = LONGTYPE - ISPOINTER
    id.args = 0
    id.hr_syntax = "_MOUSEINPUT"
    regid

    clearid
    id.n = "_MouseX"
    id.subfunc = 1
    id.callname = "func__mousex"
    id.ret = SINGLETYPE - ISPOINTER
    id.args = 0
    id.hr_syntax = "_MOUSEX"
    regid

    clearid
    id.n = "_MouseY"
    id.subfunc = 1
    id.callname = "func__mousey"
    id.ret = SINGLETYPE - ISPOINTER
    id.args = 0
    id.hr_syntax = "_MOUSEY"
    regid

    clearid
    id.n = "_MouseMovementX"
    id.subfunc = 1
    id.callname = "func__mousemovementx"
    id.ret = SINGLETYPE - ISPOINTER
    id.args = 0
    id.hr_syntax = "_MOUSEMOVEMENTX"
    regid

    clearid
    id.n = "_MouseMovementY"
    id.subfunc = 1
    id.callname = "func__mousemovementy"
    id.ret = SINGLETYPE - ISPOINTER
    id.args = 0
    id.hr_syntax = "_MOUSEMOVEMENTY"
    regid

    clearid
    id.n = "_MouseButton"
    id.subfunc = 1
    id.callname = "func__mousebutton"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_MOUSEBUTTON(buttonNumber)"
    regid

    clearid
    id.n = "_MouseWheel"
    id.subfunc = 1
    id.callname = "func__mousewheel"
    id.ret = LONGTYPE - ISPOINTER
    id.args = 0
    id.hr_syntax = "_MOUSEWHEEL"
    regid

    clearid
    id.n = "FreeFile"
    id.subfunc = 1
    id.callname = "func_freefile"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_FREEFILE"
    regid

    clearid
    id.n = "Name"
    id.subfunc = 2
    id.callname = "sub_name"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "?{As}?"
    id.hr_syntax = "NAME oldFileOrFolderName$ AS newFileOrFolderName$"
    regid

    clearid
    id.n = "Kill"
    id.subfunc = 2
    id.callname = "sub_kill"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "KILL fileSpec$"
    regid

    clearid
    id.n = "ChDir"
    id.subfunc = 2
    id.callname = "sub_chdir"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "CHDIR path$"
    regid

    clearid
    id.n = "MkDir"
    id.subfunc = 2
    id.callname = "sub_mkdir"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "MKDIR pathSpec$"
    regid

    clearid
    id.n = "RmDir"
    id.subfunc = 2
    id.callname = "sub_rmdir"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "RMDIR path$"
    regid

    clearid
    id.n = "Chain"
    id.subfunc = 2
    id.callname = "sub_chain"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "CHAIN moduleName$"
    regid

    clearid
    id.n = "Shell"
    id.subfunc = 2
    id.callname = "sub_shell2"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "{_Hide}[{_DontWait}][?]"
    id.secondargmustbe = "_Hide"
    id.hr_syntax = "SHELL [_DONTWAIT] [_HIDE] commandToRun$"
    regid

    clearid
    id.n = "Shell"
    id.subfunc = 2
    id.callname = "sub_shell3"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "{_DontWait}[{_Hide}][?]"
    id.secondargmustbe = "_DontWait"
    id.hr_syntax = "SHELL [_DONTWAIT] [_HIDE] commandToRun$"
    regid

    clearid
    id.n = "Shell"
    id.subfunc = 2
    id.callname = "sub_shell"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    'id.secondargcantbe = "_HIDE"
    id.hr_syntax = "SHELL [_DONTWAIT] [_HIDE] commandToRun$"
    regid

    clearid
    id.n = "Shell"
    id.subfunc = 1
    id.callname = "func_shell"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = INTEGER64TYPE - ISPOINTER
    id.hr_syntax = "SHELL(commandToRun$)"
    regid

    clearid
    id.n = "_ShellHide"
    id.subfunc = 1
    id.callname = "func__shellhide"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = INTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_SHELLHIDE(commandToRun$)"
    regid

    clearid
    id.n = "Command"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_command"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "COMMAND$[(index%)]"
    regid

    clearid
    id.n = "_CommandCount"
    id.subfunc = 1
    id.callname = "func__commandcount"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_COMMANDCOUNT"
    regid

    ' QB64-PE Audio Library

    clearid
    id.n = "Beep"
    id.subfunc = 2
    id.callname = "sub_beep" ' BEEP no longers pulls in miniaudio and other parts of the audio library
    id.args = 0
    id.hr_syntax = "BEEP"
    regid

    clearid
    id.n = "Play": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub_play"
    id.args = 4
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "?[,[?][,[?][,?]]]"
    id.hr_syntax = "PLAY voiceCommands1$[, voiceCommands2$][, voiceCommands3$][, voiceCommands4$]"
    regid

    clearid
    id.n = "Play": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func_play"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "PLAY [voice&]"
    regid

    clearid
    id.n = "Sound": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub_sound"
    id.args = 8
    id.arg = MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?,?[,[?][,[?][,[?][,[?][,?]]]]]][{Wait|Resume}]"
    id.hr_syntax = "SOUND [frequency!, duration![, volume!][, panPosition!][, waveform&][, waveformParameters!][, voice&]] [{Wait|Resume}]"
    regid

    clearid
    id.n = "_Wave": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__wave"
    id.args = 3
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(-3) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "?,?[,?]"
    id.hr_syntax = "_WAVE voice&, waveDefinition%%([index&])[, frameCount&]"
    regid

    clearid
    id.n = "_SndRate": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndrate"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SNDRATE"
    regid

    clearid
    id.n = "_SndRaw": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndraw"
    id.args = 3
    id.arg = MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,[?][,?]]"
    id.hr_syntax = "_SNDRAW leftSample[, rightSample][, pipeHandle&]"
    regid

    clearid
    id.n = "_SndRawBatch": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndrawbatch"
    id.args = 4
    id.arg = MKL$(-3) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "?[,[?][,[?][,?]]]"
    id.hr_syntax = "_SNDRAWBATCH sampleFrameArray!([index&])[, channels&][, pipeHandle&][, frameCount&]"
    regid

    clearid
    id.n = "_SndRawDone": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndrawdone"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "_SNDRAWDONE [pipeHandle&]"
    regid

    clearid
    id.n = "_SndOpenRaw": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndopenraw"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SNDOPENRAW"
    regid

    clearid
    id.n = "_SndRawLen": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndrawlen"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "_SNDRAWLEN [pipeHandle&]"
    regid

    clearid
    id.n = "_SndLen": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndlen"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "_SNDLEN(handle&)"
    regid

    clearid
    id.n = "_SndPaused": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndpaused"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SNDPAUSED(handle&)"
    regid

    clearid
    id.n = "_SndPlayFile": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndplayfile"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER)
    id.specialformat = "?[,[?][,?]]"
    id.hr_syntax = "_SNDPLAYFILE fileName$[, , volume!]"
    regid

    clearid
    id.n = "_SndPlayCopy": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndplaycopy"
    id.args = 5
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER)
    id.specialformat = "?[,[?][,[?][,[?][,[?]]]]]"
    id.hr_syntax = "_SNDPLAYCOPY handle&[, volume!][, x!][, y!][, z!]"
    regid

    clearid
    id.n = "_SndStop": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndstop"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "_SNDSTOP handle&"
    regid

    clearid
    id.n = "_SndLoop": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndloop"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "_SNDLOOP handle&"
    regid

    clearid
    id.n = "_SndLimit": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndlimit"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER)
    id.hr_syntax = "_SNDLIMIT handle&, numberOfSeconds#"
    regid

    clearid
    id.n = "_SndOpen": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndopen"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SNDOPEN(fileName$[, capabilities$])"
    regid

    clearid
    id.n = "_SndSetPos": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndsetpos"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(DOUBLETYPE - ISPOINTER)
    id.hr_syntax = "_SNDSETPOS handle&, position#"
    regid

    clearid
    id.n = "_SndGetPos": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndgetpos"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "_SNDGETPOS(handle&)"
    regid

    clearid
    id.n = "_SndPlaying": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndplaying"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SNDPLAYING(handle&)"
    regid

    clearid
    id.n = "_SndPause": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndpause"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "_SNDPAUSE handle&"
    regid

    clearid
    id.n = "_SndBal": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndbal"
    id.args = 5
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,[?][,[?][,[?][,[?]]]]"
    id.hr_syntax = "_SNDBAL handle&[, x!][, y!][, z!]"
    regid

    clearid
    id.n = "_SndVol": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndvol"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER)
    id.hr_syntax = "_SNDVOL handle&, volume!"
    regid

    clearid
    id.n = "_SndPlay": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndplay"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "_SNDPLAY handle&"
    regid

    clearid
    id.n = "_SndCopy": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndcopy"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SNDCOPY(handle&)"
    regid

    clearid
    id.n = "_SndClose": id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__sndclose"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "_SNDCLOSE handle&"
    regid

    clearid
    id.n = "_SndNew"
    id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 1
    id.callname = "func__sndnew"
    id.args = 3
    id.arg = MKL$(ULONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SNDNEW(frames&, channels&, bits&)"
    regid

    clearid
    id.n = "_MIDISoundBank"
    id.Dependency = DEPENDENCY_MINIAUDIO
    id.subfunc = 2
    id.callname = "sub__midisoundbank"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.hr_syntax = "_MIDISOUNDBANK fileName$[, capabilities$]"
    regid

    clearid
    id.n = "Input"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_input"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "INPUT$(numberOfBytes%[, fileOrPortNumber])"
    regid

    clearid
    id.n = "Seek"
    id.subfunc = 2
    id.callname = "sub_seek"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[#]?,?"
    id.hr_syntax = "SEEK filenumber&, position"
    regid

    clearid
    id.n = "Seek"
    id.subfunc = 1
    id.callname = "func_seek"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "SEEK(filenumber&)"
    regid

    clearid
    id.n = "Loc"
    id.subfunc = 1
    id.callname = "func_loc"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "LOC(fileOrPortNumber%)"
    regid

    clearid
    id.n = "EOF"
    id.subfunc = 1
    id.callname = "func_eof"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "EOF(fileNumber&)"
    regid

    clearid
    id.n = "LOF"
    id.subfunc = 1
    id.callname = "func_lof"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "LOF(fileNumber&)"
    regid

    clearid
    id.n = "Screen"
    id.subfunc = 1
    id.callname = "func_screen"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?[,?]"
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "SCREEN(x, y[, colorflag%])"
    regid

    clearid
    id.n = "PMap"
    id.subfunc = 1
    id.callname = "func_pmap"
    id.args = 2
    id.arg = MKL$(SINGLETYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = SINGLETYPE - ISPOINTER
    id.hr_syntax = "PMAP(coordinate, function_number%)"
    regid

    clearid
    id.n = "Point"
    id.subfunc = 1
    id.callname = "func_point"
    id.args = 2
    id.arg = MKL$(SINGLETYPE - ISPOINTER) + MKL$(SINGLETYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "POINT(x, y) or POINT({0|1|2|3})"
    regid

    clearid
    id.n = "Tab"
    id.subfunc = 1
    id.callname = "func_tab"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "TAB(column%)"
    regid

    clearid
    id.n = "Spc"
    id.subfunc = 1
    id.callname = "func_spc"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "SPC(count%)"
    regid

    clearid
    id.n = "Wait"
    id.subfunc = 2
    id.callname = "sub_wait"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,?[,?]"
    id.hr_syntax = "WAIT port%, andMask%[, xorMask%]"
    regid

    clearid
    id.n = "Inp"
    id.subfunc = 1
    id.callname = "func_inp"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "INP(address)"
    regid

    clearid
    id.n = "Pos"
    id.subfunc = 1
    id.callname = "func_pos"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "POS(0)"
    regid

    clearid
    id.n = "Sgn"
    id.subfunc = 1
    id.callname = "func_sgn"
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "SGN(value)"
    regid

    clearid
    id.n = "LBound"
    id.subfunc = 1
    id.args = 2
    id.arg = MKL$(-1) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "LBOUND(arrayName[, dimension%])"
    regid

    clearid
    id.n = "UBound"
    id.subfunc = 1
    id.args = 2
    id.arg = MKL$(-1) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "UBOUND(arrayName[, dimension%])"
    regid

    clearid
    id.n = "_Bin"
    id.musthave = "$"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_BIN$(number)"
    regid

    clearid
    id.n = "Oct"
    id.musthave = "$"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "OCT$(number)"
    regid

    clearid
    id.n = "Hex"
    id.musthave = "$"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "HEX$(decimalNumber)"
    regid

    clearid
    id.n = "Sleep"
    id.subfunc = 2
    id.callname = "sub_sleep"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.hr_syntax = "SLEEP seconds%"
    regid

    clearid
    id.n = "Exp"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = -1
    id.hr_syntax = "EXP(numericExpression)"
    regid

    clearid
    id.n = "Fix"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = -1
    id.hr_syntax = "FIX(expression)"
    regid

    clearid
    id.n = "Int"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = -1
    id.hr_syntax = "INT(expression)"
    regid

    clearid
    id.n = "CDbl"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "CDBL(expression)"
    regid

    clearid
    id.n = "CSng"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = SINGLETYPE - ISPOINTER
    id.hr_syntax = "CSNG(expression)"
    regid

    clearid
    id.n = "_Round"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = INTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_ROUND(number)"
    regid

    clearid
    id.n = "CInt"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = INTEGERTYPE - ISPOINTER
    id.hr_syntax = "CINT(expression)"
    regid

    clearid
    id.n = "CLng"
    id.subfunc = 1
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = INTEGERTYPE - ISPOINTER
    id.hr_syntax = "CLNG(expression)"
    regid

    clearid
    id.n = "Time"
    id.musthave = "$"
    id.subfunc = 2
    id.callname = "sub_time"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "=?"
    id.hr_syntax = "TIME$ = timeToSet$"
    regid

    clearid
    id.n = "Time"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_time"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "TIME$"
    regid

    clearid
    id.n = "Date"
    id.musthave = "$"
    id.subfunc = 2
    id.callname = "sub_date"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "=?"
    id.hr_syntax = "DATE$ = dateToSet$"
    regid

    clearid
    id.n = "Date"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_date"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "DATE$"
    regid

    clearid
    id.n = "CsrLin"
    id.subfunc = 1
    id.callname = "func_csrlin"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "CSRLIN"
    regid


    clearid
    id.n = "Paint"
    id.subfunc = 2
    id.callname = "sub_paint"
    id.args = 5
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?)[,[?][,[?][,?]]]"
    'PAINT [STEP] (x!,y!)[,[paint] [,[bordercolor&] [,background$]]]
    id.hr_syntax = "PAINT [STEP] (x!, y!)[,[fillColor&] [,[bordercolor&] [,pattern$]]]"
    regid

    clearid
    id.n = "Circle"
    id.subfunc = 2
    id.callname = "sub_circle"
    id.args = 7
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?),?[,[?][,[?][,[?][,?]]]]"
    'CIRCLE [STEP] (x!,y!),radius![,[color&] [,[start!] [,[end!] [,aspect!]]]]
    id.hr_syntax = "CIRCLE [STEP] (x!, y!), radius![, [color&] [, [start!] [, [end!] [, aspect!]]]]"
    regid

    clearid
    id.n = "BLoad"
    id.subfunc = 2
    id.callname = "sub_bload"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.hr_syntax = "BLOAD fileName$, VARPTR(imageArray%(index))"
    regid

    clearid
    id.n = "BSave"
    id.subfunc = 2
    id.callname = "sub_bsave"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "BSAVE saveFile$, VARPTR(array(index)), fileSize&"
    regid

    'double definition
    clearid
    id.n = "Get"
    id.subfunc = 2
    id.callname = "sub_graphics_get"
    id.args = 6
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(-3) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?)-[{Step}](?,?),?[,?]"
    id.secondargmustbe = "Step"
    id.hr_syntax = "GET [STEP] (column1, row1)-[STEP](column2, row2), array([index])[, offscreenColor]"
    regid

    clearid
    id.n = "Get"
    id.subfunc = 2
    id.callname = "sub_graphics_get"
    id.args = 6
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(-3) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?)-[{Step}](?,?),?[,?]"
    id.secondargmustbe = "("
    id.hr_syntax = "GET [STEP] (column1, row1)-[STEP](column2, row2), array([index])[, offscreenColor]"
    regid

    'double definition
    clearid
    id.n = "Put"
    id.subfunc = 2
    id.callname = "sub_graphics_put"
    id.args = 5
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(-3) + MKL$(LONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?),?[,[{_Clip}][{PSet|PReset|And|Or|Xor}][,?]]"
    'PUT [STEP] (x!,y!),arrayname# [(indexes%)] [,actionverb]
    'PUT (10, 10), myimage, _CLIP, 0
    id.secondargmustbe = "Step"
    id.hr_syntax = "PUT [STEP](column, row), Array([index])[,] [_CLIP]  [{PSET|PRESET|AND|OR|XOR}]][, omitcolor]"
    regid

    clearid
    id.n = "Put"
    id.subfunc = 2
    id.callname = "sub_graphics_put"
    id.args = 5
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(-3) + MKL$(LONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?),?[,[{_Clip}][{PSet|PReset|And|Or|Xor}][,?]]"
    'PUT [STEP] (x!,y!),arrayname# [(indexes%)] [,actionverb]
    'PUT (10, 10), myimage, _CLIP, 0
    id.secondargmustbe = "("
    id.hr_syntax = "PUT [STEP](column, row), Array([index])[,] [_CLIP]  [{PSET|PRESET|AND|OR|XOR}]][, omitcolor]"
    regid

    clearid
    id.n = "Get"
    id.subfunc = 2
    id.callname = "sub_get"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(-4)
    'id.specialformat = "[#]?,[?],?" 'non field compliant definition
    id.specialformat = "[#]?[,[?][,?]]" 'field compliant definition
    id.hr_syntax = "GET #fileNumber&, [position][, targetVariable|targetArray()}]"
    regid

    clearid
    id.n = "Put"
    id.subfunc = 2
    id.callname = "sub_put"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(-4)
    'id.specialformat = "[#]?,[?],?" 'non field compliant definition
    id.specialformat = "[#]?[,[?][,?]]" 'field compliant definition
    id.hr_syntax = "PUT #filenumber&, [position][, {holdingvariable|holdingarray()}]"
    regid

    clearid
    id.n = "Open"
    id.subfunc = 2
    id.callname = "sub_open"
    id.args = 6
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[{For Random|For Binary|For Input|For Output|For Append}][{Access Read Write|Access Read|Access Write}][{Shared|Lock Read Write|Lock Read|Lock Write}]{As}[#]?[{Len =}?]"
    id.hr_syntax = "OPEN fileName$ [FOR mode] [ACCESS|LOCK|SHARED [{READ|WRITE}] AS [#]fileNumber& [LEN = recordLength]"
    regid

    clearid
    id.n = "Open"
    id.subfunc = 2
    id.callname = "sub_open_gwbasic"
    id.args = 4
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?,[#]?,?[,?]"
    id.hr_syntax = "OPEN modeLetter$, [#]fileNumber&, fileName$[, recordLength]"
    regid

    clearid
    id.n = "Val"
    id.subfunc = 1
    id.callname = "func_val"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "VAL(string_value$)"
    regid

    clearid
    id.n = "MKSMBF"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_mksmbf"
    id.args = 1
    id.arg = MKL$(SINGLETYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "MKSMBF$(value!)"
    regid

    clearid
    id.n = "MKDMBF"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_mkdmbf"
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "MKDMBF$(value#)"
    regid

    clearid
    id.n = "MKI"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(INTEGERTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "MKI$(integerVariableOrLiteral%)"
    regid

    clearid
    id.n = "MKL"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "MKL$(longVariableOrLiteral&)"
    regid

    clearid
    id.n = "MKS"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(SINGLETYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "MKS$(singlePrecisionVariableOrLiteral!)"
    regid

    clearid
    id.n = "MKD"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "MKD$(doublePrecisionVariableOrLiteral#)"
    regid

    clearid
    id.n = "_MK"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = ""
    id.args = 2
    id.arg = MKL$(-1) + MKL$(-1)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_MK$(numericalType, numericalValue)"
    regid

    clearid
    id.n = "CVSMBF"
    id.subfunc = 1
    id.callname = "func_cvsmbf"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = SINGLETYPE - ISPOINTER
    id.hr_syntax = "CVSMBF(stringData$)"
    regid

    clearid
    id.n = "CVDMBF"
    id.subfunc = 1
    id.callname = "func_cvdmbf"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "CVDMBF(stringData$)"
    regid

    clearid
    id.n = "CVI"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = INTEGERTYPE - ISPOINTER
    id.hr_syntax = "CVI(stringData$)"
    regid

    clearid
    id.n = "CVL"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "CVL(stringData$)"
    regid

    clearid
    id.n = "CVS"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = SINGLETYPE - ISPOINTER
    id.hr_syntax = "CVS(stringData$)"
    regid

    clearid
    id.n = "CVD"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "CVD(stringData$)"
    regid

    clearid
    id.n = "_CV"
    id.subfunc = 1
    id.callname = ""
    id.args = 2
    id.arg = MKL$(-1) + MKL$(STRINGTYPE - ISPOINTER)
    id.ret = -1
    id.hr_syntax = "_CV(numericalType, MKstringValue$)"
    regid

    clearid
    id.n = "String"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_string"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "STRING$(count&, {character$ | ASCIIcode%})"
    regid

    clearid
    id.n = "Space"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_space"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "SPACE$(count&)"
    regid

    clearid
    id.n = "InStr"
    id.subfunc = 1
    id.callname = "func_instr"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?],?,?" 'checked!
    id.hr_syntax = "INSTR([start%,] baseString$, searchString$)"
    regid

    clearid
    id.n = "_InStrRev"
    id.subfunc = 1
    id.callname = "func__instrrev"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.specialformat = "[?],?,?" 'checked!
    id.hr_syntax = "_INSTRREV([start%,] baseString$, searchString$)"
    regid

    clearid
    id.n = "Mid"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_mid"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.specialformat = "?,?,[?]" 'checked!
    id.hr_syntax = "MID$(stringValue$, startPosition%[, bytes%])"
    regid

    clearid
    id.n = "SAdd"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(-1) '!this value is ignored, the qb64 compiler handles this function
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "SADD(stringVariable$)"
    regid

    clearid
    id.n = "Cls"
    id.subfunc = 2
    id.callname = "sub_clsDest"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,?]]"
    id.hr_syntax = "CLS [method%][, bgColor&][, dest&]"
    regid

    clearid
    id.n = "Sqr"
    id.subfunc = 1
    id.callname = "func_sqr"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "SQR(value)"
    regid

    clearid
    id.n = "Chr"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func_chr"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "CHR$(code%)"
    regid

    clearid
    id.n = "VarPtr"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(-1) '!this value is ignored, the qb64 compiler handles this function
    id.ret = STRINGTYPE - ISPOINTER
    id.musthave = "$"
    id.hr_syntax = "VARPTR(variable_name[(reference_index%)])"
    regid

    clearid
    id.n = "VarPtr"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(-1) '!this value is ignored, the qb64 compiler handles this function
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "VARPTR(variable_name[(reference_index%)])"
    regid

    clearid
    id.n = "_Offset"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(-1) '!this value is ignored, the qb64 compiler handles this function
    id.ret = UOFFSETTYPE - ISPOINTER
    id.hr_syntax = "_OFFSET(variable)"
    regid

    clearid
    id.n = "VarSeg"
    id.subfunc = 1
    id.callname = ""
    id.args = 1
    id.arg = MKL$(-1) '!this value is ignored, the qb64 compiler handles this function
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "VARSEG(variable_name[(start_index)])"
    regid

    clearid
    id.n = "Poke"
    id.subfunc = 2
    id.callname = "sub_poke"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "POKE segment_offset, offset_value"
    regid

    clearid
    id.n = "Peek"
    id.subfunc = 1
    id.callname = "func_peek"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "PEEK(segment_offset)"
    regid

    clearid
    id.n = "Def"
    id.subfunc = 2
    id.callname = "sub_defseg"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{Seg}[=?]" 'checked!
    id.secondargmustbe = "Seg"
    id.hr_syntax = "DEF SEG [=][{segment|VARSEG(variable}]"
    regid

    clearid
    id.n = "Sin"
    id.subfunc = 1
    id.callname = "std::sin"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "SIN(radian_angle!)"
    regid

    clearid
    id.n = "Cos"
    id.subfunc = 1
    id.callname = "std::cos"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "COS(radian_angle!)"
    regid

    clearid
    id.n = "Tan"
    id.subfunc = 1
    id.callname = "std::tan"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "TAN(radian_angle!)"
    regid

    clearid
    id.n = "Atn"
    id.subfunc = 1
    id.callname = "std::atan"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "ATN(tangent!)"
    regid

    clearid
    id.n = "Log"
    id.subfunc = 1
    id.callname = "func_log"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "LOG(value)"
    regid

    clearid
    id.n = "Abs"
    id.subfunc = 1
    id.callname = "func_abs"
    id.args = 1
    id.arg = MKL$(-1) 'takes anything numerical
    id.ret = FLOATTYPE - ISPOINTER '***overridden by function evaluatefunc***
    id.hr_syntax = "ABS(numericalValue)"
    regid

    clearid
    id.n = "Erl"
    id.subfunc = 1
    id.callname = "get_error_erl"
    id.args = 0
    id.ret = DOUBLETYPE - ISPOINTER
    id.hr_syntax = "ERL"
    regid

    clearid
    id.n = "Err"
    id.subfunc = 1
    id.callname = "get_error_err"
    id.args = 0
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "ERR"
    regid

    clearid
    id.n = "Error"
    id.subfunc = 2
    id.callname = "error"
    id.args = 1
    id.arg = MKL$(ULONGTYPE - ISPOINTER)
    id.hr_syntax = "ERROR codeNumber%"
    regid

    clearid
    id.n = "Line"
    id.subfunc = 2
    id.callname = "sub_line"
    id.args = 7
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[[{Step}](?,?)]-[{Step}](?,?)[,[?][,[{B|BF}][,?]]]"
    id.hr_syntax = "LINE [STEP] [(column1, row1)]-[STEP] (column2, row2), color[, [{B|BF}], style%]"
    regid

    clearid
    id.n = "Timer"
    id.subfunc = 1
    id.callname = "func_timer"
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.ret = SINGLETYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "TIMER[(accuracy!)]"
    regid

    clearid
    id.n = "Rnd"
    id.subfunc = 1
    id.callname = "func_rnd"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = SINGLETYPE - ISPOINTER
    id.specialformat = "[?]" 'checked!
    id.hr_syntax = "RND[(behavior)]"
    regid

    clearid
    id.n = "Randomize"
    id.subfunc = 2
    id.callname = "sub_randomize"
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.specialformat = "[[{Using}]?]" 'checked!
    id.hr_syntax = "RANDOMIZE [USING] {seednumber|TIMER}"
    regid

    clearid
    id.n = "Out"
    id.subfunc = 2
    id.callname = "sub_out"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "OUT registerAddress%, value%"
    regid

    clearid
    id.n = "PCopy"
    id.subfunc = 2
    id.callname = "sub_pcopy"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.hr_syntax = "PCOPY sourcePage%, destinationPage%"
    regid

    clearid
    id.n = "View"
    id.subfunc = 2
    id.callname = "qbg_sub_view"
    id.args = 6
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[[{Screen}](?,?)-(?,?)[,[?][,?]]]"
    id.secondargcantbe = "Print"
    id.hr_syntax = "VIEW [SCREEN] (column1, row1)-(column2, row2)[, color][, border]"
    regid

    clearid
    id.n = "View"
    id.subfunc = 2
    id.callname = "qbg_sub_view_print"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "{Print}[?{To}?]" 'new!
    id.secondargmustbe = "Print"
    id.hr_syntax = "VIEW PRINT [topRow% TO bottomRow%]"
    regid

    clearid
    id.n = "Window"
    id.subfunc = 2
    id.callname = "qbg_sub_window"
    id.args = 4
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER)
    id.specialformat = "[[{Screen}](?,?)-(?,?)]"
    id.hr_syntax = "WINDOW [[SCREEN] (x1!, y1!) - (x2!, y2!)]"
    regid

    clearid
    id.n = "Locate"
    id.subfunc = 2
    id.callname = "qbg_sub_locate"
    id.args = 5
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,[?][,[?][,?]]]]"
    id.hr_syntax = "LOCATE [row%][, column%] [, cursor%][, cursorStart%, cursorStop%]"
    regid

    clearid
    id.n = "Color"
    id.subfunc = 2
    id.callname = "qbg_sub_color"
    id.args = 3
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,?]]"
    id.hr_syntax = "COLOR [foreground&][, background&]"
    regid

    clearid
    id.n = "Palette"
    id.subfunc = 2
    id.callname = "qbg_palette"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?,?]"
    id.hr_syntax = "PALETTE [attribute%, red% + (green% * 256) + (blue% * 65536)] or PALETTE [existingAttribute%, newAttribute%]"
    regid

    clearid
    id.n = "Width"
    id.subfunc = 2
    id.callname = "qbsub_width"
    id.args = 5
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{#|LPrint}][?][,[?][,[?][,[?]]]]" 'new!
    'id.specialformat = "[{#|LPRINT}][?][,?]" 'new!
    id.hr_syntax = "WIDTH [columns%][, rows%] or WIDTH {file_number|device}, columnwidth%"
    regid


    clearid
    id.n = "Screen"
    id.subfunc = 2
    id.callname = "qbg_screen"
    id.args = 5
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    'id.specialformat = "[?][,[?][,[?][,?]]]" 'new!
    'id.specialformat = "[?][,[?][,[?][,[?][,{_MANUALDISPLAY}]]]]" 'breaks compilation!
    'id.specialformat = "[?][,[?][,[?][,[?][,[{_MANUALDISPLAY}]]]]]" <-pre-bulletproofing
    id.specialformat = "[?][,[?][,[?][,[?][,[{_ManualDisplay}?]]]]]" 'a temp format for transition reasons"
    id.hr_syntax = "SCREEN {mode%|imagehandle&} [, , active_page, visual_page]"
    regid

    clearid
    id.n = "PSet"
    id.subfunc = 2
    id.callname = "sub_pset"
    id.args = 3
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?)[,?]"
    id.hr_syntax = "PSET [STEP](column%, row%)[, colorAttribute]"
    regid

    clearid
    id.n = "PReset"
    id.subfunc = 2
    id.callname = "sub_preset"
    id.args = 3
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{Step}](?,?)[,?]"
    id.hr_syntax = "PRESET [STEP](column%, row%)[, colorAttribute]"
    regid

    clearid
    id.n = "Asc"
    id.subfunc = 1
    id.callname = "qbs_asc"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "ASC(text$[, position%])"
    regid

    clearid
    id.n = "Len"
    id.subfunc = 1
    id.callname = "" 'callname is not used
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER) 'note: LEN is a special case, any input is actually accepted
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "LEN(literalTextOrVariable$)"
    regid

    clearid
    id.n = "InKey"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_inkey"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "INKEY$"
    regid

    clearid
    id.n = "Str"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_str"
    id.args = 1
    id.arg = MKL$(-1)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "STR$(number)"
    regid

    clearid
    id.n = "_ToStr"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs__tostr"
    id.args = 2
    id.specialformat = "?[,?]"
    id.arg = MKL$(-1) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_TOSTR$(number[,digits])"
    regid

    clearid
    id.n = "UCase"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_ucase"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "UCASE$(text$)"
    regid

    clearid
    id.n = "LCase"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_lcase"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "LCASE$(text$)"
    regid

    clearid
    id.n = "Left"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_left"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "LEFT$(stringValue$, numberOfCharacters%)"
    regid

    clearid
    id.n = "Right"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_right"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "RIGHT$(stringValue$, numberOfCharacters%)"
    regid

    clearid
    id.n = "LTrim"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_ltrim"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "LTRIM$(text$)"
    regid

    clearid
    id.n = "RTrim"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs_rtrim"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "RTRIM$(text$)"
    regid

    clearid
    id.n = "_Trim"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "qbs__trim"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_TRIM$(text$)"
    regid

    clearid
    id.n = "Print"
    id.subfunc = 2
    id.callname = "qbs_print" 'not called directly
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "PRINT [expression] [{;|,] [expression...]"
    regid

    clearid
    id.n = "LPrint": id.Dependency = DEPENDENCY_PRINTER
    id.subfunc = 2
    id.callname = "qbs_lprint" 'not called directly
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "LPRINT [expression] [{;|,}]"
    regid

    clearid
    id.n = "LPos": id.Dependency = DEPENDENCY_PRINTER
    id.subfunc = 1
    id.callname = "func_lpos"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "LPOS(index%)"
    regid

    'Get Current Working Directory
    clearid
    id.n = "_CWD"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__cwd"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_CWD$"
    regid

    'Get the directory the program was started from (before the currenct directory is automatically changed to the executables directory)
    clearid
    id.n = "_StartDir"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__startdir"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_STARTDIR$"
    regid

    'Return a path that best represents the context provided e.g. _DIR$("DESKTOP")
    clearid
    id.n = "_Dir"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__dir"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_DIR$(" + CHR$(34) + "folderspecification" + CHR$(34) + ")"
    regid

    'Return the name of the included file in which the last error occurred
    clearid
    id.n = "_InclErrorFile"
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__inclerrorfile"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_INCLERRORFILE$"
    regid

    clearid
    id.n = "_KeyClear"
    id.subfunc = 2
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.callname = "sub__keyclear"
    id.hr_syntax = "_KEYCLEAR buffer&"
    regid

    clearid
    id.n = "_D2R"
    id.subfunc = 1
    id.callname = "func_deg2rad"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_D2R(angleInDegrees!)"
    regid

    clearid
    id.n = "_D2G"
    id.subfunc = 1
    id.callname = "func_deg2grad"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_D2G(angleInDegrees!)"
    regid

    clearid
    id.n = "_R2D"
    id.subfunc = 1
    id.callname = "func_rad2deg"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_R2D(angleInRadians!)"
    regid

    clearid
    id.n = "_R2G"
    id.subfunc = 1
    id.callname = "func_rad2grad"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_R2G(angleInRadians!)"
    regid

    clearid
    id.n = "_G2D"
    id.subfunc = 1
    id.callname = "func_grad2deg"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_G2D(gradient!)"
    regid

    clearid
    id.n = "_G2R"
    id.subfunc = 1
    id.callname = "func_grad2rad"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_G2R(gradient!)"
    regid

    clearid 'Clear the old id info so we set the slate for a new one
    id.n = "_Atan2" 'The name of our new one
    id.subfunc = 1 'And this is a function
    id.callname = "std::atan2" 'The C name of the function
    id.args = 2 'It takes 2 parameters to work
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) 'These simply add up to represent the 2 parameters from what I can tell
    id.ret = FLOATTYPE - ISPOINTER 'we want it to return to us a nice _FLOAT value
    id.hr_syntax = "_ATAN2(y, x)"
    regid 'and we're finished with ID registration

    clearid 'Clear the old id info so we set the slate for a new one
    id.n = "_Hypot" 'The name of our new one
    id.subfunc = 1 'And this is a function
    id.callname = "std::hypot" 'The C name of the function
    id.args = 2 'It takes 2 parameters to work
    id.arg = MKL$(FLOATTYPE - ISPOINTER) + MKL$(FLOATTYPE - ISPOINTER) 'These simply add up to represent the 2 parameters from what I can tell
    id.ret = FLOATTYPE - ISPOINTER 'we want it to return to us a nice _FLOAT value
    id.hr_syntax = "_HYPOT(x, y)"
    regid 'and we're finished with ID registration

    clearid
    id.n = "_Asin"
    id.subfunc = 1
    id.callname = "std::asin"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ASIN(sine_value!)"
    regid

    clearid
    id.n = "_Acos"
    id.subfunc = 1
    id.callname = "std::acos"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ACOS(cosine_value!)"
    regid

    clearid
    id.n = "_Sinh"
    id.subfunc = 1
    id.callname = "std::sinh"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_SINH(value)"
    regid

    clearid
    id.n = "_Cosh"
    id.subfunc = 1
    id.callname = "std::cosh"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_COSH(value)"
    regid

    clearid
    id.n = "_Tanh"
    id.subfunc = 1
    id.callname = "std::tanh"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_TANH(value)"
    regid

    clearid
    id.n = "_Asinh"
    id.subfunc = 1
    id.callname = "std::asinh"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ASINH(value)"
    regid

    clearid
    id.n = "_Acosh"
    id.subfunc = 1
    id.callname = "std::acosh"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ACOSH(value)"
    regid

    clearid
    id.n = "_Atanh"
    id.subfunc = 1
    id.callname = "std::atanh"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ATANH(value)"
    regid

    clearid
    id.n = "_Ceil"
    id.subfunc = 1
    id.callname = "std::ceil"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_CEIL(expression)"
    regid

    clearid
    id.n = "_Pi"
    id.subfunc = 1
    id.callname = "func_pi"
    id.args = 1
    id.arg = MKL$(DOUBLETYPE - ISPOINTER)
    id.ret = DOUBLETYPE - ISPOINTER
    id.specialformat = "[?]"
    id.hr_syntax = "_PI[(multiplier)]"
    regid

    clearid
    id.n = "_DesktopHeight"
    id.subfunc = 1
    id.callname = "func_screenheight"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_DESKTOPHEIGHT"
    regid

    clearid
    id.n = "_DesktopWidth"
    id.subfunc = 1
    id.callname = "func_screenwidth"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_DESKTOPWIDTH"
    regid

    clearid
    id.n = "_ScreenIcon" 'name change to from _ICONIFYWINDOW to _SCREENICON to match the screenshow and screenhide
    id.subfunc = 2
    id.callname = "sub_screenicon"
    id.hr_syntax = "_SCREENICON"
    regid

    clearid
    id.n = "_ScreenExists"
    id.subfunc = 1
    id.callname = "func_windowexists"
    id.hr_syntax = "_SCREENEXISTS"
    regid

    clearid
    id.n = "_ControlChr"
    id.subfunc = 1
    id.callname = "func__controlchr"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CONTROLCHR"
    regid

    clearid
    id.n = "_StriCmp"
    id.subfunc = 1
    id.callname = "func__str_nc_compare"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_STRICMP(string1$, string2$)"
    regid

    clearid
    id.n = "_StrCmp"
    id.subfunc = 1
    id.callname = "func__str_compare"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_STRCMP(string1$, string2$)"
    regid

    clearid
    id.n = "_Arcsec"
    id.subfunc = 1
    id.callname = "func_arcsec"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ARCSEC(value)"
    regid

    clearid
    id.n = "_Arccsc"
    id.subfunc = 1
    id.callname = "func_arccsc"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ARCCSC(value)"
    regid

    clearid
    id.n = "_Arccot"
    id.subfunc = 1
    id.callname = "func_arccot"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_ARCCOT(value)"
    regid

    clearid
    id.n = "_Sech"
    id.subfunc = 1
    id.callname = "func_sech"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_SECH(value)"
    regid

    clearid
    id.n = "_Csch"
    id.subfunc = 1
    id.callname = "func_csch"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_CSCH(value)"
    regid

    clearid
    id.n = "_Coth"
    id.subfunc = 1
    id.callname = "func_coth"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_COTH(value)"
    regid

    clearid
    id.n = "_Sec"
    id.subfunc = 1
    id.callname = "func_sec"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_SEC(value)"
    regid

    clearid
    id.n = "_Csc"
    id.subfunc = 1
    id.callname = "func_csc"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_CSC(value)"
    regid

    clearid
    id.n = "_Cot"
    id.subfunc = 1
    id.callname = "func_cot"
    id.args = 1
    id.arg = MKL$(FLOATTYPE - ISPOINTER)
    id.ret = FLOATTYPE - ISPOINTER
    id.hr_syntax = "_COT(value)"
    regid

    clearid
    id.n = "_ScreenIcon"
    id.subfunc = 1
    id.callname = "func_screenicon"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCREENICON"
    regid

    clearid
    id.n = "_AutoDisplay"
    id.subfunc = 1
    id.callname = "func__autodisplay"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_AUTODISPLAY"
    regid

    clearid
    id.n = "_ShR"
    id.subfunc = 1
    id.callname = "func__shr"
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = UINTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_SHR(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_ShL"
    id.subfunc = 1
    id.callname = "func__shl"
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = UINTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_SHL(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_RoL"
    id.subfunc = 1
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) ' ***overridden by function evaluatefunc***
    id.ret = UINTEGER64TYPE - ISPOINTER ' ***overridden by function evaluatefunc***
    id.hr_syntax = "_ROL(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_RoR"
    id.subfunc = 1
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER) ' ***overridden by function evaluatefunc***
    id.ret = UINTEGER64TYPE - ISPOINTER ' ***overridden by function evaluatefunc***
    id.hr_syntax = "_ROR(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_Adler32"
    id.Dependency = DEPENDENCY_ZLIB
    id.subfunc = 1
    id.callname = "func__adler32"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_ADLER32(dataString$)"
    regid

    clearid
    id.n = "_Crc32"
    id.Dependency = DEPENDENCY_ZLIB
    id.subfunc = 1
    id.callname = "func__crc32"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = ULONGTYPE - ISPOINTER
    id.hr_syntax = "_CRC32(dataString$)"
    regid

    clearid
    id.n = "_Deflate"
    id.Dependency = DEPENDENCY_ZLIB
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__deflate"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_DEFLATE$(stringToCompress$[, compressionLevel&])"
    regid

    clearid
    id.n = "_Inflate"
    id.Dependency = DEPENDENCY_ZLIB
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__inflate"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(INTEGER64TYPE - ISPOINTER)
    id.specialformat = "?[,?]"
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_INFLATE$(stringToDecompress$[, originalSize&])"
    regid

    clearid
    id.n = "_Md5"
    id.Dependency = DEPENDENCY_LOADFONT
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__md5"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_MD5$(dataString$)"
    regid

    clearid
    id.n = "_Base64Decode"
    id.Dependency = DEPENDENCY_ZLIB
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__base64decode"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_BASE64DECODE$(stringToDecode$)"
    regid

    clearid
    id.n = "_Base64Encode"
    id.Dependency = DEPENDENCY_ZLIB
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__base64encode"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_BASE64ENCODE$(stringToEncode$)"
    regid

    clearid
    id.n = "_Embedded"
    id.Dependency = DEPENDENCY_EMBED
    id.musthave = "$"
    id.subfunc = 1
    id.callname = "func__embedded"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.ret = STRINGTYPE - ISPOINTER
    id.hr_syntax = "_EMBEDDED$(" + AddQuotes$("handle") + ")"
    regid

    clearid
    id.n = "_CInp"
    id.subfunc = 1
    id.callname = "func__cinp"
    id.args = 1
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?]"
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CINP"
    regid

    clearid
    id.n = "_CapsLock"
    id.subfunc = 1
    id.callname = "func__capslock"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CAPSLOCK"
    regid

    clearid
    id.n = "_ScrollLock"
    id.subfunc = 1
    id.callname = "func__scrolllock"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_SCROLLLOCK"
    regid

    clearid
    id.n = "_NumLock"
    id.subfunc = 1
    id.callname = "func__numlock"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_NUMLOCK"
    regid

    clearid
    id.n = "_CapsLock"
    id.subfunc = 2
    id.callname = "sub__capslock"
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.args = 1
    id.specialformat = "{On|Off|_Toggle}"
    id.hr_syntax = "_CAPSLOCK {On|Off|_Toggle}"
    regid

    clearid
    id.n = "_Scrolllock"
    id.subfunc = 2
    id.callname = "sub__scrolllock"
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.args = 1
    id.specialformat = "{On|Off|_Toggle}"
    id.hr_syntax = "_SCROLLLOCK {On|Off|_Toggle}"
    regid

    clearid
    id.n = "_Numlock"
    id.subfunc = 2
    id.callname = "sub__numlock"
    id.arg = MKL$(LONGTYPE - ISPOINTER)
    id.args = 1
    id.specialformat = "{On|Off|_Toggle}"
    id.hr_syntax = "_NUMLOCK {On|Off|_Toggle}"
    regid

    clearid
    id.n = "_ConsoleFont"
    id.subfunc = 2
    id.callname = "sub__consolefont"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(INTEGERTYPE - ISPOINTER)
    id.hr_syntax = "_CONSOLEFONT fontFile$"
    regid

    clearid
    id.n = "_ConsoleCursor"
    id.subfunc = 2
    id.callname = "sub__console_cursor"
    id.args = 2
    id.arg = MKL$(LONGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[{_Show|_Hide}][,?]"
    id.hr_syntax = "_CONSOLECURSOR {_Show|_Hide}[, size%]"
    regid

    clearid
    id.n = "_ConsoleInput"
    id.subfunc = 1
    id.callname = "func__getconsoleinput"
    id.args = 0
    id.ret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_CONSOLEINPUT"
    regid

    clearid
    id.n = "_ReadBit"
    id.subfunc = 1
    id.callname = "func__readbit"
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = INTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_READBIT(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_SetBit"
    id.subfunc = 1
    id.callname = "func__setbit"
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = UINTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_SETBIT(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_ResetBit"
    id.subfunc = 1
    id.callname = "func__resetbit"
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = UINTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_RESETBIT(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_ToggleBit"
    id.subfunc = 1
    id.callname = "func__togglebit"
    id.args = 2
    id.arg = MKL$(UINTEGER64TYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.ret = UINTEGER64TYPE - ISPOINTER
    id.hr_syntax = "_TOGGLEBIT(numericalVariable, numericalValue)"
    regid

    clearid
    id.n = "_NotifyPopup"
    id.subfunc = 2 ' 1 = function, 2 = sub
    id.callname = "sub__guiNotifyPopup"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,?]]"
    id.hr_syntax = "_NOTIFYPOPUP [title$][, message$][, iconType$]"
    regid

    clearid
    id.n = "_MessageBox"
    id.subfunc = 2 ' 1 = function, 2 = sub
    id.callname = "sub__guiMessageBox"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,?]]"
    id.hr_syntax = "_MESSAGEBOX [title$][, message$][, iconType$]"
    regid

    clearid
    id.n = "_SelectFolderDialog"
    id.musthave = "$"
    id.subfunc = 1 ' 1 = function, 2 = sub
    id.callname = "func__guiSelectFolderDialog"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?][,?]"
    id.ret = STRINGTYPE - ISPOINTER ' return type for functions
    id.hr_syntax = "_SELECTFOLDERDIALOG$([title$][, defaultPath$])"
    regid

    clearid
    id.n = "_ColorChooserDialog"
    id.subfunc = 1 ' 1 = function, 2 = sub
    id.callname = "func__guiColorChooserDialog"
    id.args = 2
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(ULONGTYPE - ISPOINTER)
    id.specialformat = "[?][,?]"
    id.ret = ULONGTYPE - ISPOINTER ' return type for functions
    id.hr_syntax = "_COLORCHOOSERDIALOG&([title$][, defaultRGB&])"
    regid

    clearid
    id.n = "_MessageBox"
    id.subfunc = 1 ' 1 = function, 2 = sub
    id.callname = "func__guiMessageBox"
    id.args = 5
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,[?][,[?][,?]]]]"
    id.ret = LONGTYPE - ISPOINTER ' return type for functions
    id.hr_syntax = "_MESSAGEBOX&([title$][, message$][, dialogType$][, iconType$][, defaultButton&])"
    regid

    clearid
    id.n = "_InputBox"
    id.musthave = "$"
    id.subfunc = 1 ' 1 = function, 2 = sub
    id.callname = "func__guiInputBox"
    id.args = 3
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,?]]"
    id.ret = STRINGTYPE - ISPOINTER ' return type for functions
    id.hr_syntax = "_INPUTBOX$([title$][, message$][, defaultInput$])"
    regid

    clearid
    id.n = "_OpenFileDialog"
    id.musthave = "$"
    id.subfunc = 1 ' 1 = function, 2 = sub
    id.callname = "func__guiOpenFileDialog"
    id.args = 5
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(LONGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,[?][,[?][,?]]]]"
    id.ret = STRINGTYPE - ISPOINTER ' return type for functions
    id.hr_syntax = "_OPENFILEDIALOG$([title$][, defaultPathAndFile$][, filterPatterns$][, singleFilterDescription$][, allowMultipleSelects&])"
    regid

    clearid
    id.n = "_SaveFileDialog"
    id.musthave = "$"
    id.subfunc = 1 ' 1 = function, 2 = sub
    id.callname = "func__guiSaveFileDialog"
    id.args = 4
    id.arg = MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER) + MKL$(STRINGTYPE - ISPOINTER)
    id.specialformat = "[?][,[?][,[?][,?]]]"
    id.ret = STRINGTYPE - ISPOINTER ' return type for functions
    id.hr_syntax = "_SAVEFILEDIALOG$([title$][, defaultPathAndFile$][, filterPatterns$][, singleFilterDescription$])"
    regid

    clearid
    id.n = "_LogTrace"
    id.subfunc = 2
    id.callname = "sub__log_trace"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_LOGTRACE msg$"
    regid

    clearid
    id.n = "_LogInfo"
    id.subfunc = 2
    id.callname = "sub__log_info"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_LOGINFO msg$"
    regid

    clearid
    id.n = "_LogWarn"
    id.subfunc = 2
    id.callname = "sub__log_warn"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_LOGWARN msg$"
    regid

    clearid
    id.n = "_LogError"
    id.subfunc = 2
    id.callname = "sub__log_error"
    id.args = 1
    id.arg = MKL$(STRINGTYPE - ISPOINTER)
    id.hr_syntax = "_LOGERROR msg$"
    regid

    clearid
    id.n = "_LogMinLevel"
    id.subfunc = 1
    id.callname = "func__logminlevel"
    id.args = 0
    id.arg = ""
    ideret = LONGTYPE - ISPOINTER
    id.hr_syntax = "_LOGMINLEVEL"
    regid

    reginternalsubfunc = 0
END SUB

