10 PRINT "CHARACTER BUFFER EXAMPLE"
20 PRINT "========================"
30 PRINT
40 REM Create a simple string in memory at address 4000
50 REM Store character codes: H=72, E=69, L=76, O=79
60 POKE 4000, 72
70 POKE 4001, 69
80 POKE 4002, 76
90 POKE 4003, 76
100 POKE 4004, 79
110 POKE 4005, 0
120 PRINT "Stored character codes for 'HELLO'"
130 PRINT
140 REM Read them back and display
150 PRINT "Character codes stored:"
160 FOR I = 0 TO 4
170 LET C = PEEK(4000 + I)
180 PRINT "Position "; I; " code: "; C
190 NEXT I
200 END
