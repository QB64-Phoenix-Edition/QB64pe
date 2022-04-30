CLS
'All of my Declarations
DIM selection AS INTEGER
DIM upper AS STRING
DIM LCASE2 AS STRING
DIM lower AS STRING
DIM UCASE2 AS STRING
'End of Declarations
start:

PRINT "Uppercase ----> Lowercase & Lowercase ----> Uppercase Conversion"
PRINT
PRINT "1) Uppercase ----> Lowercase"
PRINT
PRINT "2) Lowercase ----> Uppercase Conversion"
PRINT
INPUT "Enter your choice"; selection

SELECT CASE selection

   CASE 1
   CLS
   PRINT "Uppercase ----> Lowercase Conversion"
   PRINT
   INPUT "Enter Uppercase character"; upper
   PRINT
   LCASE2 = LCASE$(upper)
   PRINT "The character in Lowercase is:"; LCASE2

   CASE 2
   CLS
   PRINT "Lowercase ----> Uppercase Conversion"
   PRINT
   INPUT "Enter Lowercase character"; lower
   PRINT
   UCASE2 = UCASE$(lower)
   PRINT "The character in Uppercase is:"; UCASE2

CASE ELSE

PRINT "Invalid Selection"
GOTO start

END SELECT
