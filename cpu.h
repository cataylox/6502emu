#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// CPU Status flags
#define FLAG_C 0x01  // Carry
#define FLAG_Z 0x02  // Zero
#define FLAG_I 0x04  // Interrupt Disable
#define FLAG_D 0x08  // Decimal Mode
#define FLAG_B 0x10  // Break Command
#define FLAG_U 0x20  // Unused (always 1)
#define FLAG_V 0x40  // Overflow
#define FLAG_N 0x80  // Negative

typedef struct {
    uint8_t A;      // Accumulator
    uint8_t X;      // X register
    uint8_t Y;      // Y register
    uint8_t SP;     // Stack pointer
    uint16_t PC;    // Program counter
    uint8_t status; // Status register
    uint64_t cycles; // Total cycles executed
} CPU;

void cpu_init(CPU *cpu);
void cpu_reset(CPU *cpu);
void cpu_step(CPU *cpu);
void cpu_execute(CPU *cpu, uint64_t max_cycles);

#endif
