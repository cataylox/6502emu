#include "memory.h"
#include <string.h>

#define MEMORY_SIZE 65536

static uint8_t memory[MEMORY_SIZE];

void memory_init(void) {
    memset(memory, 0, MEMORY_SIZE);
}

uint8_t memory_read(uint16_t address) {
    return memory[address];
}

void memory_write(uint16_t address, uint8_t value) {
    memory[address] = value;
}

uint16_t memory_read_word(uint16_t address) {
    return memory[address] | (memory[address + 1] << 8);
}
