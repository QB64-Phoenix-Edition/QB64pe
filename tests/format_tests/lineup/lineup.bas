$leT   wind  = 0
$Let   maco  = 1

$if    wind  then
'--------------------------------------------------------------------
type   test
x      as   _byte '---
xx     as   _byte     '----
xxx    as   _byte   '---
xxxx   as   _byte
end    type

'    $inclUDE   :   'included.file'
    $EmbED   :   '.\included.file'     ,      'test'

scrEEN 13
for    x=1 to 10
circle (     x     ,     x     )     ,     x
nEXt   X
?"done"
?      "done"
?
if ((las%>=65 and las%<=90) or (las%>=97 and las%<=122)) and _
   ((oas%>=65 and oas%<=90) or (oas%>=97 and oas%<=122)) and _
   (abs(las%-oas%)=32) then mid$(layout$,lcnt,1)=och$
end

data   for,"to",step,   1,2,3,   &H1,&B1,&O1

sub    Hallo
a=0
b$="
c$="xxx
_delay 0.2
end    sub
'--------------------------------------------------------------------
$else  if   maco      then
'--------------------------------------------------------------------
type   test
x      as   _byte '---
xx     as   _byte     '----
xxx    as   _byte   '---
xxxx   as   _byte
end    type

'    $inclUDE   :   'included.file'
    $EmbED   :   '.\included.file'     ,      'test'

scrEEN 13
for    x=1 to 10
circle (     x     ,     x     )     ,     x
nEXt   X
?"done"
?      "done"
?
if ((las%>=65 and las%<=90) or (las%>=97 and las%<=122)) and _
   ((oas%>=65 and oas%<=90) or (oas%>=97 and oas%<=122)) and _
   (abs(las%-oas%)=32) then mid$(layout$,lcnt,1)=och$
end

data   for,"to",step,   1,2,3,   &H1,&B1,&O1

sub    Hallo
a=0
b$="
c$="xxx
_delay 0.2
end    sub
'--------------------------------------------------------------------
$else
'--------------------------------------------------------------------
type   test
x      as   _byte '---
xx     as   _byte     '----
xxx    as   _byte   '---
xxxx   as   _byte
end    type

'    $inclUDE   :   'included.file'
    $EmbED   :   '.\included.file'     ,      'test'

scrEEN 13
for    x=1 to 10
circle (     x     ,     x     )     ,     x
nEXt   X
?"done"
?      "done"
?
if ((las%>=65 and las%<=90) or (las%>=97 and las%<=122)) and _
   ((oas%>=65 and oas%<=90) or (oas%>=97 and oas%<=122)) and _
   (abs(las%-oas%)=32) then mid$(layout$,lcnt,1)=och$
end

data   for,"to",step,   1,2,3,   &H1,&B1,&O1

sub    Hallo
a=0
b$="
c$="xxx
_delay 0.2
end    sub
'--------------------------------------------------------------------
$endif

