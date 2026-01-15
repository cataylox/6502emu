10 PRINT "DATA STORAGE EXAMPLE"
20 PRINT "===================="
30 PRINT
40 REM Store a sequence of numbers in memory
50 LET A = 5000
60 FOR I = 0 TO 9
70 POKE A + I, I * 10
80 NEXT I
90 PRINT "Stored 0,10,20...90 at addresses 5000-5009"
100 PRINT
110 REM Read them back
120 PRINT "Reading back:"
130 FOR I = 0 TO 9
140 LET V = PEEK(A + I)
150 PRINT "Address "; A + I; " = "; V
160 NEXT I
170 PRINT
180 PRINT "Sum of values:"
190 LET S = 0
200 FOR I = 0 TO 9
210 LET S = S + PEEK(A + I)
220 NEXT I
230 PRINT "Total = "; S
240 END
