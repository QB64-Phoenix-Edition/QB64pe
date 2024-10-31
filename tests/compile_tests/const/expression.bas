$CONSOLE:ONLY

CONST const__OR = 1 OR 2
CONST const__AND = 3 AND 1
CONST const__NOT = NOT 2
CONST const__XOR = 2 XOR 3
CONST const__EQV = 10 EQV 5
CONST const__IMP = 20 IMP 50

CONST const__mod = 20 MOD 3
CONST const__add = 2 + 3
CONST const__sub = 2 - 3
CONST const__div = 6 / 3
CONST const__idiv = 7 \ 3
CONST const__mult = 7 * 20
CONST const_pow = 3 ^ 10

CONST const__negate = -20

CONST const__eq = 2 = 2
CONST const__neq = 2 <> 3
CONST const__neq2 = 2 >< 3
CONST const__leq = 2 <= 3
CONST const__leq2 = 2 =< 3
CONST const__geq = 2 >= 3
CONST const__geq2 = 2 => 3
CONST const__gt = 2 > 3
CONST const__lt = 2 < 3

' The left side which has no parens to indicate order should still equal the
' right side which has parens to enforce order.
CONST const__oporder1 = (2 ^ 2 * 2) = ((2 ^ 2) * 2)
CONST const__oporder2 = (2 ^ 2 + 2) = ((2 ^ 2) + 2)
CONST const__oporder3 = (NOT 2 + 3) = (NOT (2 + 3))
CONST const__oporder4 = (-2 ^ 2) = (-(2 ^ 2))
CONST const__oporder5 = (NOT 2 ^ 3) = (NOT (2 ^ 3))
CONST const__oporder6 = (3 * 6 / 2) = ((3 * 6) / 2)
CONST const__oporder7 = (3 * 10 \ 3) = ((3 * 10) \ 3)

' Many levels of parens
CONST const__parens = (2 ^ (3 * (4 - (2 - (10 / (20 / 2))))))

CONST const__str = "foobar"
CONST const__str2 = "foobar" + "foobar2"
CONST const__str3 = const__str + const__str2
CONST const__str4 = (const__str + (const__str2))

CONST const__unsignedint = 2~&& * 5~&&

CONST const__division_floating = 1& / 5&

CONST const__negate_floating = -.25!

PRINT const__OR
PRINT const__AND
PRINT const__NOT
PRINT const__XOR
PRINT const__EQV
PRINT const__IMP

PRINT const__mod
PRINT const__add
PRINT const__sub
PRINT const__div
PRINT const__idiv
PRINT const__mult
PRINT const_pow

PRINT const__negate

PRINT const__eq
PRINT const__neq
PRINT const__neq2
PRINT const__leq
PRINT const__leq2
PRINT const__geq
PRINT const__geq2
PRINT const__gt
PRINT const__lt

PRINT const__oporder1
PRINT const__oporder2
PRINT const__oporder3
PRINT const__oporder4
PRINT const__oporder5
PRINT const__oporder6
PRINT const__oporder7

PRINT const__parens

PRINT const__str
PRINT const__str2
PRINT const__str3
PRINT const__str4

PRINT const__unsignedint

PRINT const__division_floating

PRINT const__negate_floating

SYSTEM
