$Console:Only

Type ReturnProbe
    mem As _MEM
    addr As _Offset
    i64 As _Integer64
    text As String
    code As String * 3
    wide As _Float
End Type

Dim a As ReturnProbe
Dim b As ReturnProbe
Dim r As ReturnProbe
Dim directMem As _MEM

Dim gotA As Long
Dim gotB As Long
Dim gotLoop As Long
Dim gotDirect As Long

Dim i As Long
Dim marker As Long
Dim haveR As _Byte

' Two independent returned UDT values containing _MEM, _OFFSET,
' _INTEGER64, _FLOAT, variable STRING and STRING * 3.
a = MakeProbe(16, 5000000000&&, 1.25, "Alpha" + Chr$(0) + "Omega", "A1!", 111111)
b = MakeProbe(24, -6000000000&&, 2.5, "Beta" + Chr$(0) + "Data", "B2!", 222222)

_MemGet a.mem, a.mem.OFFSET, gotA
_MemGet b.mem, b.mem.OFFSET, gotB

If a.mem.SIZE <> 16 Then Print "FAIL Func_Ret_UDT_MEM004 a.mem.SIZE": System 1
If b.mem.SIZE <> 24 Then Print "FAIL Func_Ret_UDT_MEM004 b.mem.SIZE": System 1
If a.addr <> a.mem.OFFSET Then Print "FAIL Func_Ret_UDT_MEM004 a.addr": System 1
If b.addr <> b.mem.OFFSET Then Print "FAIL Func_Ret_UDT_MEM004 b.addr": System 1
If a.i64 <> 5000000000&& Then Print "FAIL Func_Ret_UDT_MEM004 a.i64": System 1
If b.i64 <> -6000000000&& Then Print "FAIL Func_Ret_UDT_MEM004 b.i64": System 1
If a.wide <> 1.25 Then Print "FAIL Func_Ret_UDT_MEM004 a.wide": System 1
If b.wide <> 2.5 Then Print "FAIL Func_Ret_UDT_MEM004 b.wide": System 1
If a.text <> "Alpha" + Chr$(0) + "Omega" Then Print "FAIL Func_Ret_UDT_MEM004 a.text": System 1
If b.text <> "Beta" + Chr$(0) + "Data" Then Print "FAIL Func_Ret_UDT_MEM004 b.text": System 1
If Len(a.text) <> 11 Then Print "FAIL Func_Ret_UDT_MEM004 LEN(a.text)": System 1
If Len(b.text) <> 9 Then Print "FAIL Func_Ret_UDT_MEM004 LEN(b.text)": System 1
If a.code <> "A1!" Then Print "FAIL Func_Ret_UDT_MEM004 a.code": System 1
If b.code <> "B2!" Then Print "FAIL Func_Ret_UDT_MEM004 b.code": System 1
If gotA <> 111111 Then Print "FAIL Func_Ret_UDT_MEM004 a.mem payload": System 1
If gotB <> 222222 Then Print "FAIL Func_Ret_UDT_MEM004 b.mem payload": System 1
If a.mem.OFFSET = b.mem.OFFSET Then Print "FAIL Func_Ret_UDT_MEM004 returned _MEM blocks alias": System 1

' Variable STRING ownership must be independent.
a.text = "Changed A"
If b.text <> "Beta" + Chr$(0) + "Data" Then Print "FAIL Func_Ret_UDT_MEM004 STRING ownership alias": System 1

' Reuse one FUNCTION call site repeatedly. This exercises reuse of the
' hidden UDT result storage and its owned variable STRING member.
For i = 1 To 5
    If haveR Then
        _MemFree r.mem
        haveR = 0
    End If

    marker = 300000 + i

    r = MakeProbe(32, 7000000000&& + i, i + .5, _
        "Loop-" + LTrim$(Str$(i)), _
        Chr$(64 + i) + "!!", _
        marker)

    haveR = -1

    _MemGet r.mem, r.mem.OFFSET, gotLoop

    If r.mem.SIZE <> 32 Then Print "FAIL Func_Ret_UDT_MEM004 loop mem.SIZE at"; i: System 1
    If r.addr <> r.mem.OFFSET Then Print "FAIL Func_Ret_UDT_MEM004 loop _OFFSET at"; i: System 1
    If r.i64 <> 7000000000&& + i Then Print "FAIL Func_Ret_UDT_MEM004 loop _INTEGER64 at"; i: System 1
    If r.wide <> i + .5 Then Print "FAIL Func_Ret_UDT_MEM004 loop _FLOAT at"; i: System 1
    If r.text <> "Loop-" + LTrim$(Str$(i)) Then Print "FAIL Func_Ret_UDT_MEM004 loop STRING at"; i: System 1
    If r.code <> Chr$(64 + i) + "!!" Then Print "FAIL Func_Ret_UDT_MEM004 loop STRING * 3 at"; i: System 1
    If gotLoop <> marker Then Print "FAIL Func_Ret_UDT_MEM004 loop _MEM payload at"; i: System 1
Next

If haveR Then
    _MemFree r.mem
    haveR = 0
End If

' Direct member access from a returned UDT expression exercises the ABI
' property that the FUNCTION result pointer can be used directly by .member.
directMem = MakeProbe(12, 9000000000&&, 4.5, "Direct", "DIR", 424242).mem

_MemGet directMem, directMem.OFFSET, gotDirect

If directMem.SIZE <> 12 Then Print "FAIL Func_Ret_UDT_MEM004 direct member _MEM size": System 1
If gotDirect <> 424242 Then Print "FAIL Func_Ret_UDT_MEM004 direct member _MEM payload": System 1

_MemFree directMem
_MemFree a.mem
_MemFree b.mem

Print "PASS Func_Ret_UDT_MEM004"
System

Function MakeProbe (bytes As Long, value64 As _Integer64, valueFloat As _Float, valueText As String, valueCode As String, marker As Long) As ReturnProbe
    Dim temp As ReturnProbe

    temp.mem = _MemNew(bytes)
    _MemPut temp.mem, temp.mem.OFFSET, marker

    temp.addr = temp.mem.OFFSET
    temp.i64 = value64
    temp.wide = valueFloat
    temp.text = valueText
    temp.code = valueCode

    MakeProbe = temp
End Function
