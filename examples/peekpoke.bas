10 PRINT "PEEK AND POKE TEST"
20 PRINT "=================="
30 PRINT
40 REM Write some values to memory
50 POKE 1000, 42
60 POKE 1001, 123
70 POKE 1002, 255
80 PRINT "Wrote 42 to address 1000"
90 PRINT "Wrote 123 to address 1001"
100 PRINT "Wrote 255 to address 1002"
110 PRINT
120 REM Read values back
130 LET A = PEEK(1000)
140 LET B = PEEK(1001)
150 LET C = PEEK(1002)
160 PRINT "Read from address 1000: "; A
170 PRINT "Read from address 1001: "; B
180 PRINT "Read from address 1002: "; C
190 PRINT
200 REM Test expression in PEEK
210 LET X = 1000
220 LET Y = PEEK(X + 1)
230 PRINT "PEEK(1000 + 1) = "; Y
240 PRINT
250 REM Test byte overflow (255 + 1 = 0)
260 POKE 2000, 256
270 LET Z = PEEK(2000)
280 PRINT "POKE 2000, 256 then PEEK: "; Z
290 PRINT
300 PRINT "Test completed!"
310 END
