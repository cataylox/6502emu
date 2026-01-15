# 6502 CPU Emulator & BASIC Interpreter

A simple 6502 CPU emulator with a Microsoft-style BASIC interpreter written in C.

## Features

### 6502 Emulator
- Full 6502 instruction set implementation
- 64KB memory space
- Accurate cycle counting
- All addressing modes supported
- Status flag handling

### BASIC Interpreter
- Microsoft 6502 BASIC compatible syntax
- Variables (A-Z, single letter)
- Arithmetic expressions (+, -, *, /, parentheses)
- Commands: PRINT, LET, INPUT, GOTO, IF/THEN, FOR/NEXT, REM, END
- Relational operators: =, <, >, <=, >=, <>
- String literals in PRINT statements

## Building

```bash
make
```

This builds both:
- `6502emu` - Pure 6502 emulator test
- `6502basic` - BASIC interpreter

## Running

Run the CPU emulator test:
```bash
make run
```

Run the BASIC interpreter:
```bash
make runbasic
```

Or directly:
```bash
./6502basic
```

## BASIC Language Reference

### Supported Commands

- `PRINT` - Output text and numbers
  - `PRINT "Hello"` - Print string
  - `PRINT A` - Print variable
  - `PRINT A; B` - Print multiple items
  - `PRINT A,` - Tab separator

- `LET` - Assign variables
  - `LET A = 10`
  - `LET B = A + 5`

- `INPUT` - Read user input
  - `INPUT A`
  - `INPUT "Enter value: "; A`

- `IF/THEN` - Conditional execution
  - `IF A > 10 THEN PRINT "Big"`
  - `IF A = B THEN GOTO 100`

- `FOR/NEXT` - Loops
  - `FOR I = 1 TO 10`
  - `NEXT I`

- `GOTO` - Jump to line number
  - `GOTO 100`

- `REM` - Comments
  - `REM This is a comment`

- `END` - End program

### Example Programs

```basic
10 PRINT "Hello, World!"
20 END
```

```basic
10 FOR I = 1 TO 10
20 PRINT I
30 NEXT I
40 END
```

```basic
10 INPUT "Enter a number: "; A
20 LET B = A * 2
30 PRINT "Double is: "; B
40 END
```

## Architecture

- `cpu.h/c` - CPU emulation with instruction execution
- `memory.h/c` - 64KB memory array with read/write functions
- `basic.h/c` - BASIC interpreter
- `main.c` - CPU emulator test
- `main_basic.c` - BASIC interpreter main program
