#include <stdio.h>
#include "cpu.h"
#include "memory.h"

int main() {
    CPU cpu;
    
    memory_init();
    cpu_init(&cpu);
    
    // Example program: Add two numbers
    memory_write(0x0000, 0xA9); // LDA #$05
    memory_write(0x0001, 0x05);
    memory_write(0x0002, 0x69); // ADC #$03
    memory_write(0x0003, 0x03);
    memory_write(0x0004, 0x85); // STA $10
    memory_write(0x0005, 0x10);
    memory_write(0x0006, 0x00); // BRK
    
    cpu.PC = 0x0000;
    
    printf("Initial state:\n");
    printf("PC: 0x%04X  A: 0x%02X  X: 0x%02X  Y: 0x%02X  SP: 0x%02X  Status: 0x%02X\n",
           cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.status);
    
    // Execute program
    for (int i = 0; i < 10 && cpu.PC < 0x0007; i++) {
        cpu_step(&cpu);
        printf("PC: 0x%04X  A: 0x%02X  X: 0x%02X  Y: 0x%02X  SP: 0x%02X  Status: 0x%02X  Cycles: %lu\n",
               cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.status, cpu.cycles);
    }
    
    printf("\nResult at $10: 0x%02X (should be 0x08)\n", memory_read(0x10));
    
    return 0;
}
