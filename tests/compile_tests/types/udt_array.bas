$Console:Only

'Test assignment to and size of numeric UDT array
Type num_t
    a As Long
    b As Long
End Type
Dim na(-3 To 2) As num_t
na(-3).a = -12345
na(-3).b = -54321
na(2).a = 12345
na(2).b = 54321
Print "NA VALUES: "; na(-3).a; na(-3).b; na(2).a; na(2).b
Print "NA SIZE: "; Len(na())


'Test resizing dynamic array of numeric UDT initialises to 0
ReDim nda(0 To 2) As num_t
nda(0).a = -12345
nda(0).b = -54321
nda(2).a = 12345
nda(2).b = 54321
ReDim nda(0 To 1) As num_t
Print "NDA VALUES1: "; nda(0).a; nda(0).b; nda(1).a; nda(1).b
ReDim nda(0 To 2) As num_t
Print "NDA VALUES2: "; nda(0).a; nda(0).b; nda(1).a; nda(1).b; nda(2).a; nda(2).b


'Test resizing _preserve dynamic array of numeric UDT initialises to 0
ReDim _Preserve ndpa(0 To 2) As num_t
ndpa(0).a = -12345
ndpa(0).b = -54321
ndpa(2).a = 12345
ndpa(2).b = 54321
ReDim _Preserve ndpa(0 To 1) As num_t
Print "NDPA VALUES1: "; ndpa(0).a; ndpa(0).b; ndpa(1).a; ndpa(1).b
ndpa(1).a = 56789
ndpa(1).b = 98765
ReDim _Preserve ndpa(0 To 2) As num_t
Print "NDPA VALUES2: "; ndpa(0).a; ndpa(0).b; ndpa(1).a; ndpa(1).b; ndpa(2).a; ndpa(2).b


'Test assignment to variable string UDT array
Type str_t
    a As Long
    s As String
    b As Long
End Type
Dim sa(-3 To 2) As str_t
sa(-3).a = -12345
sa(-3).s = "hello"
sa(-3).b = -54321
sa(2).a = 12345
sa(2).s = "strings"
sa(2).b = 54321
Print "SA VALUES: "; sa(-3).a; sa(-3).s; sa(-3).b; sa(2).a; sa(2).s; sa(2).b


'Test resizing dynamic array of variable string UDT initialises to 0 / empty string
ReDim sda(0 To 2) As str_t
sda(0).a = -12345
sda(0).s = "hello"
sda(0).b = -54321
sda(2).a = 12345
sda(2).s = "strings"
sda(2).b = 54321
ReDim sda(0 To 1) As str_t
Print "SDA VALUES1: "; sda(0).a; sda(0).s; sda(0).b; sda(1).a; sda(1).s; sda(1).b
ReDim sda(0 To 2) As str_t
Print "SDA VALUES2: "; sda(0).a; sda(0).s; sda(0).b; sda(1).a; sda(1).s; sda(1).b; sda(2).a; sda(2).s; sda(2).b


'Test resizing _preserve dynamic array of variable string UDT initialises new elements to 0 / empty string
ReDim _Preserve sdpa(0 To 2) As str_t
sdpa(0).a = -12345
sdpa(0).s = "hello"
sdpa(0).b = -54321
sdpa(2).a = 12345
sdpa(2).s = "strings"
sdpa(2).b = 54321
ReDim _Preserve sdpa(0 To 1) As str_t
Print "SDPA VALUES1: "; sdpa(0).a; sdpa(0).s; sdpa(0).b; sdpa(1).a; sdpa(1).s; sdpa(1).b
sdpa(1).a = 56789
sdpa(1).s = "more"
sdpa(1).b = 98765
ReDim _Preserve sdpa(0 To 2) As str_t
Print "SDPA VALUES2: "; sdpa(0).a; sdpa(0).s; sdpa(0).b; sdpa(1).a; sdpa(1).s; sdpa(1).b; sdpa(2).a; sdpa(2).s; sdpa(2).b

System
