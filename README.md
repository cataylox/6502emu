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
- Commands: PRINT, LET, INPUT, GOTO, IF/THEN, FOR/NEXT, REM, END, PEEK, POKE
- Relational operators: =, <, >, <=, >=, <>
- String literals in PRINT statements
- Memory access with PEEK and POKE

## Building

```bash
make
```

This builds both:
- `6502emu` - Pure 6502 emulator test
- `6502basic` - BASIC interpreter

## Running

### CPU Emulator

Run the built-in CPU emulator test:
```bash
make run
```

Or directly:
```bash
./6502emu
```

#### Loading Binary Programs

Load and execute a binary file:
```bash
./6502emu --load program.bin
```

Load a binary file at a specific memory offset (hex):
```bash
./6502emu --load program.bin --offset 0x2000
```

Load a binary file at a specific memory offset (decimal):
```bash
./6502emu --load program.bin --offset 8192
```

Display help and usage information:
```bash
./6502emu --help
```

**Command-Line Options:**
- `--load FILE` - Load binary FILE into emulator memory
- `--offset OFFSET` - Load file at memory OFFSET (hexadecimal or decimal)
  - Default: 0x0000
  - Range: 0x0000 to 0xFFFF (0 to 65535)
  - Examples: `0x2000`, `8192`
- `--help` - Display help message

**Notes:**
- When `--load` is used, the program counter (PC) starts at the specified offset
- The emulator executes up to 1000 instructions or until a BRK (0x00) instruction
- Files are loaded as raw binary data (machine code)
- If only the emulator is run without `--load`, it executes a built-in test program

### BASIC Interpreter

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

- `PEEK` - Read byte from memory
  - `PEEK(address)` - Returns byte value (0-255) at memory address (0-65535)
  - `LET A = PEEK(1000)` - Read from address 1000

- `POKE` - Write byte to memory
  - `POKE address, value` - Write byte value to address
  - `POKE 1000, 42` - Write 42 to address 1000

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

```basic
10 REM PEEK and POKE example
20 POKE 1000, 42
30 LET A = PEEK(1000)
40 PRINT "Value at address 1000: "; A
50 END
```

## Architecture

- `cpu.h/c` - CPU emulation with instruction execution
- `memory.h/c` - 64KB memory array with read/write functions
- `basic.h/c` - BASIC interpreter
- `main.c` - CPU emulator with command-line interface
- `main_basic.c` - BASIC interpreter main program

## Creating Binary Programs

To create a binary program for the emulator, you can:

1. **Hand-assemble machine code** - Write 6502 opcodes directly as bytes
2. **Use an assembler** - Use tools like ca65 (from cc65) or other 6502 assemblers
3. **Create with a script** - Generate binary files programmatically

### Example: Simple Binary Program

Create a file with this Python script to generate a simple program:

```python
#!/usr/bin/env python3
# Example: Load 0x42 into accumulator and stop
program = [
    0xA9, 0x42,  # LDA #$42 - Load immediate value 0x42
    0x00         # BRK - Break/stop
]

with open('simple.bin', 'wb') as f:
    f.write(bytes(program))
```

Run it:
```bash
python3 create_program.py
./6502emu --load simple.bin
```

Expected output shows A register = 0x42 after execution.
