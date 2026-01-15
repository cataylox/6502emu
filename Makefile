CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = 6502emu
BASIC_TARGET = 6502basic
OBJS = main.o cpu.o memory.o
BASIC_OBJS = main_basic.o basic.o cpu.o memory.o

all: $(TARGET) $(BASIC_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(BASIC_TARGET): $(BASIC_OBJS)
	$(CC) $(CFLAGS) -o $(BASIC_TARGET) $(BASIC_OBJS)

main.o: main.c cpu.h memory.h
	$(CC) $(CFLAGS) -c main.c

main_basic.o: main_basic.c basic.h
	$(CC) $(CFLAGS) -c main_basic.c

basic.o: basic.c basic.h cpu.h memory.h
	$(CC) $(CFLAGS) -c basic.c

cpu.o: cpu.c cpu.h memory.h
	$(CC) $(CFLAGS) -c cpu.c

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c memory.c

clean:
	rm -f $(OBJS) $(BASIC_OBJS) $(TARGET) $(BASIC_TARGET)

run: $(TARGET)
	./$(TARGET)

runbasic: $(BASIC_TARGET)
	./$(BASIC_TARGET)

.PHONY: all clean run runbasic
