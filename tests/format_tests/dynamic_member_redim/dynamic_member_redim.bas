$Unstable:TypeFields

Type BufferType
    sample(1, 1) _Dynamic As Long
End Type

ReDim buffers(1) As BufferType
ReDim buffers(1).sample(-5 To -3, -2 To -1)
ReDim buffers(1).sample(firstIndex - 5 To lastIndex - 3, -2 To -1)
