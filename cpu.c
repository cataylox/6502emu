#include "cpu.h"
#include "memory.h"
#include <stdio.h>

// Helper macros
#define SET_FLAG(cpu, flag) ((cpu)->status |= (flag))
#define CLR_FLAG(cpu, flag) ((cpu)->status &= ~(flag))
#define GET_FLAG(cpu, flag) ((cpu)->status & (flag))
#define SET_ZN(cpu, val) do { \
    if ((val) == 0) SET_FLAG(cpu, FLAG_Z); else CLR_FLAG(cpu, FLAG_Z); \
    if ((val) & 0x80) SET_FLAG(cpu, FLAG_N); else CLR_FLAG(cpu, FLAG_N); \
} while(0)

// Stack operations
#define STACK_BASE 0x0100
#define PUSH(cpu, val) memory_write(STACK_BASE + (cpu)->SP--, (val))
#define PULL(cpu) memory_read(STACK_BASE + ++(cpu)->SP)

void cpu_init(CPU *cpu) {
    cpu->A = 0;
    cpu->X = 0;
    cpu->Y = 0;
    cpu->SP = 0xFD;
    cpu->PC = 0;
    cpu->status = FLAG_U | FLAG_I;
    cpu->cycles = 0;
}

void cpu_reset(CPU *cpu) {
    cpu->SP = 0xFD;
    cpu->status = FLAG_U | FLAG_I;
    cpu->PC = memory_read_word(0xFFFC);
    cpu->cycles = 0;
}

// Addressing modes
static uint16_t addr_immediate(CPU *cpu) { return cpu->PC++; }
static uint16_t addr_zeropage(CPU *cpu) { return memory_read(cpu->PC++); }
static uint16_t addr_zeropage_x(CPU *cpu) { return (memory_read(cpu->PC++) + cpu->X) & 0xFF; }
static uint16_t addr_zeropage_y(CPU *cpu) { return (memory_read(cpu->PC++) + cpu->Y) & 0xFF; }
static uint16_t addr_absolute(CPU *cpu) { uint16_t addr = memory_read_word(cpu->PC); cpu->PC += 2; return addr; }
static uint16_t addr_absolute_x(CPU *cpu) { uint16_t addr = memory_read_word(cpu->PC); cpu->PC += 2; return addr + cpu->X; }
static uint16_t addr_absolute_y(CPU *cpu) { uint16_t addr = memory_read_word(cpu->PC); cpu->PC += 2; return addr + cpu->Y; }
static uint16_t addr_indirect_x(CPU *cpu) { 
    uint8_t base = (memory_read(cpu->PC++) + cpu->X) & 0xFF;
    return memory_read(base) | (memory_read((base + 1) & 0xFF) << 8);
}
static uint16_t addr_indirect_y(CPU *cpu) { 
    uint8_t base = memory_read(cpu->PC++);
    uint16_t addr = memory_read(base) | (memory_read((base + 1) & 0xFF) << 8);
    return addr + cpu->Y;
}

// Instructions
static void LDA(CPU *cpu, uint16_t addr) { cpu->A = memory_read(addr); SET_ZN(cpu, cpu->A); }
static void LDX(CPU *cpu, uint16_t addr) { cpu->X = memory_read(addr); SET_ZN(cpu, cpu->X); }
static void LDY(CPU *cpu, uint16_t addr) { cpu->Y = memory_read(addr); SET_ZN(cpu, cpu->Y); }
static void STA(CPU *cpu, uint16_t addr) { memory_write(addr, cpu->A); }
static void STX(CPU *cpu, uint16_t addr) { memory_write(addr, cpu->X); }
static void STY(CPU *cpu, uint16_t addr) { memory_write(addr, cpu->Y); }

static void ADC(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    uint16_t sum = cpu->A + val + (GET_FLAG(cpu, FLAG_C) ? 1 : 0);
    if (sum > 0xFF) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    if (((cpu->A ^ sum) & (val ^ sum) & 0x80)) SET_FLAG(cpu, FLAG_V); else CLR_FLAG(cpu, FLAG_V);
    cpu->A = sum & 0xFF;
    SET_ZN(cpu, cpu->A);
}

static void SBC(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    uint16_t diff = cpu->A - val - (GET_FLAG(cpu, FLAG_C) ? 0 : 1);
    if (diff < 0x100) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    if (((cpu->A ^ val) & (cpu->A ^ diff) & 0x80)) SET_FLAG(cpu, FLAG_V); else CLR_FLAG(cpu, FLAG_V);
    cpu->A = diff & 0xFF;
    SET_ZN(cpu, cpu->A);
}

static void AND(CPU *cpu, uint16_t addr) { cpu->A &= memory_read(addr); SET_ZN(cpu, cpu->A); }
static void ORA(CPU *cpu, uint16_t addr) { cpu->A |= memory_read(addr); SET_ZN(cpu, cpu->A); }
static void EOR(CPU *cpu, uint16_t addr) { cpu->A ^= memory_read(addr); SET_ZN(cpu, cpu->A); }

static void CMP(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    uint16_t result = cpu->A - val;
    if (cpu->A >= val) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    SET_ZN(cpu, result & 0xFF);
}

static void CPX(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    uint16_t result = cpu->X - val;
    if (cpu->X >= val) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    SET_ZN(cpu, result & 0xFF);
}

static void CPY(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    uint16_t result = cpu->Y - val;
    if (cpu->Y >= val) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    SET_ZN(cpu, result & 0xFF);
}

static void INC(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr) + 1;
    memory_write(addr, val);
    SET_ZN(cpu, val);
}

static void DEC(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr) - 1;
    memory_write(addr, val);
    SET_ZN(cpu, val);
}

static void ASL(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    if (val & 0x80) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    val <<= 1;
    memory_write(addr, val);
    SET_ZN(cpu, val);
}

static void LSR(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    if (val & 0x01) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    val >>= 1;
    memory_write(addr, val);
    SET_ZN(cpu, val);
}

static void ROL(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    uint8_t carry = GET_FLAG(cpu, FLAG_C) ? 1 : 0;
    if (val & 0x80) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    val = (val << 1) | carry;
    memory_write(addr, val);
    SET_ZN(cpu, val);
}

static void ROR(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    uint8_t carry = GET_FLAG(cpu, FLAG_C) ? 0x80 : 0;
    if (val & 0x01) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
    val = (val >> 1) | carry;
    memory_write(addr, val);
    SET_ZN(cpu, val);
}

static void BIT(CPU *cpu, uint16_t addr) {
    uint8_t val = memory_read(addr);
    if (val & 0x80) SET_FLAG(cpu, FLAG_N); else CLR_FLAG(cpu, FLAG_N);
    if (val & 0x40) SET_FLAG(cpu, FLAG_V); else CLR_FLAG(cpu, FLAG_V);
    if ((cpu->A & val) == 0) SET_FLAG(cpu, FLAG_Z); else CLR_FLAG(cpu, FLAG_Z);
}

// Branch instructions
static void branch(CPU *cpu, int condition) {
    int8_t offset = memory_read(cpu->PC++);
    if (condition) {
        cpu->PC += offset;
        cpu->cycles++;
    }
}

void cpu_step(CPU *cpu) {
    uint8_t opcode = memory_read(cpu->PC++);
    
    switch (opcode) {
        // LDA
        case 0xA9: LDA(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0xA5: LDA(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0xB5: LDA(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0xAD: LDA(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0xBD: LDA(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        case 0xB9: LDA(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        case 0xA1: LDA(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0xB1: LDA(cpu, addr_indirect_y(cpu)); cpu->cycles += 5; break;
        
        // LDX
        case 0xA2: LDX(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0xA6: LDX(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0xB6: LDX(cpu, addr_zeropage_y(cpu)); cpu->cycles += 4; break;
        case 0xAE: LDX(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0xBE: LDX(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        
        // LDY
        case 0xA0: LDY(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0xA4: LDY(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0xB4: LDY(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0xAC: LDY(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0xBC: LDY(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        
        // STA
        case 0x85: STA(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x95: STA(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0x8D: STA(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0x9D: STA(cpu, addr_absolute_x(cpu)); cpu->cycles += 5; break;
        case 0x99: STA(cpu, addr_absolute_y(cpu)); cpu->cycles += 5; break;
        case 0x81: STA(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0x91: STA(cpu, addr_indirect_y(cpu)); cpu->cycles += 6; break;
        
        // STX
        case 0x86: STX(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x96: STX(cpu, addr_zeropage_y(cpu)); cpu->cycles += 4; break;
        case 0x8E: STX(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        
        // STY
        case 0x84: STY(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x94: STY(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0x8C: STY(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        
        // ADC
        case 0x69: ADC(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0x65: ADC(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x75: ADC(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0x6D: ADC(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0x7D: ADC(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        case 0x79: ADC(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        case 0x61: ADC(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0x71: ADC(cpu, addr_indirect_y(cpu)); cpu->cycles += 5; break;
        
        // SBC
        case 0xE9: SBC(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0xE5: SBC(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0xF5: SBC(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0xED: SBC(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0xFD: SBC(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        case 0xF9: SBC(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        case 0xE1: SBC(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0xF1: SBC(cpu, addr_indirect_y(cpu)); cpu->cycles += 5; break;
        
        // AND
        case 0x29: AND(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0x25: AND(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x35: AND(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0x2D: AND(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0x3D: AND(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        case 0x39: AND(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        case 0x21: AND(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0x31: AND(cpu, addr_indirect_y(cpu)); cpu->cycles += 5; break;
        
        // ORA
        case 0x09: ORA(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0x05: ORA(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x15: ORA(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0x0D: ORA(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0x1D: ORA(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        case 0x19: ORA(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        case 0x01: ORA(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0x11: ORA(cpu, addr_indirect_y(cpu)); cpu->cycles += 5; break;
        
        // EOR
        case 0x49: EOR(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0x45: EOR(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x55: EOR(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0x4D: EOR(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0x5D: EOR(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        case 0x59: EOR(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        case 0x41: EOR(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0x51: EOR(cpu, addr_indirect_y(cpu)); cpu->cycles += 5; break;
        
        // CMP
        case 0xC9: CMP(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0xC5: CMP(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0xD5: CMP(cpu, addr_zeropage_x(cpu)); cpu->cycles += 4; break;
        case 0xCD: CMP(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        case 0xDD: CMP(cpu, addr_absolute_x(cpu)); cpu->cycles += 4; break;
        case 0xD9: CMP(cpu, addr_absolute_y(cpu)); cpu->cycles += 4; break;
        case 0xC1: CMP(cpu, addr_indirect_x(cpu)); cpu->cycles += 6; break;
        case 0xD1: CMP(cpu, addr_indirect_y(cpu)); cpu->cycles += 5; break;
        
        // CPX
        case 0xE0: CPX(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0xE4: CPX(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0xEC: CPX(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        
        // CPY
        case 0xC0: CPY(cpu, addr_immediate(cpu)); cpu->cycles += 2; break;
        case 0xC4: CPY(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0xCC: CPY(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        
        // INC
        case 0xE6: INC(cpu, addr_zeropage(cpu)); cpu->cycles += 5; break;
        case 0xF6: INC(cpu, addr_zeropage_x(cpu)); cpu->cycles += 6; break;
        case 0xEE: INC(cpu, addr_absolute(cpu)); cpu->cycles += 6; break;
        case 0xFE: INC(cpu, addr_absolute_x(cpu)); cpu->cycles += 7; break;
        
        // DEC
        case 0xC6: DEC(cpu, addr_zeropage(cpu)); cpu->cycles += 5; break;
        case 0xD6: DEC(cpu, addr_zeropage_x(cpu)); cpu->cycles += 6; break;
        case 0xCE: DEC(cpu, addr_absolute(cpu)); cpu->cycles += 6; break;
        case 0xDE: DEC(cpu, addr_absolute_x(cpu)); cpu->cycles += 7; break;
        
        // ASL
        case 0x0A: if (cpu->A & 0x80) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
                   cpu->A <<= 1; SET_ZN(cpu, cpu->A); cpu->cycles += 2; break;
        case 0x06: ASL(cpu, addr_zeropage(cpu)); cpu->cycles += 5; break;
        case 0x16: ASL(cpu, addr_zeropage_x(cpu)); cpu->cycles += 6; break;
        case 0x0E: ASL(cpu, addr_absolute(cpu)); cpu->cycles += 6; break;
        case 0x1E: ASL(cpu, addr_absolute_x(cpu)); cpu->cycles += 7; break;
        
        // LSR
        case 0x4A: if (cpu->A & 0x01) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
                   cpu->A >>= 1; SET_ZN(cpu, cpu->A); cpu->cycles += 2; break;
        case 0x46: LSR(cpu, addr_zeropage(cpu)); cpu->cycles += 5; break;
        case 0x56: LSR(cpu, addr_zeropage_x(cpu)); cpu->cycles += 6; break;
        case 0x4E: LSR(cpu, addr_absolute(cpu)); cpu->cycles += 6; break;
        case 0x5E: LSR(cpu, addr_absolute_x(cpu)); cpu->cycles += 7; break;
        
        // ROL
        case 0x2A: { uint8_t carry = GET_FLAG(cpu, FLAG_C) ? 1 : 0;
                     if (cpu->A & 0x80) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
                     cpu->A = (cpu->A << 1) | carry; SET_ZN(cpu, cpu->A); cpu->cycles += 2; } break;
        case 0x26: ROL(cpu, addr_zeropage(cpu)); cpu->cycles += 5; break;
        case 0x36: ROL(cpu, addr_zeropage_x(cpu)); cpu->cycles += 6; break;
        case 0x2E: ROL(cpu, addr_absolute(cpu)); cpu->cycles += 6; break;
        case 0x3E: ROL(cpu, addr_absolute_x(cpu)); cpu->cycles += 7; break;
        
        // ROR
        case 0x6A: { uint8_t carry = GET_FLAG(cpu, FLAG_C) ? 0x80 : 0;
                     if (cpu->A & 0x01) SET_FLAG(cpu, FLAG_C); else CLR_FLAG(cpu, FLAG_C);
                     cpu->A = (cpu->A >> 1) | carry; SET_ZN(cpu, cpu->A); cpu->cycles += 2; } break;
        case 0x66: ROR(cpu, addr_zeropage(cpu)); cpu->cycles += 5; break;
        case 0x76: ROR(cpu, addr_zeropage_x(cpu)); cpu->cycles += 6; break;
        case 0x6E: ROR(cpu, addr_absolute(cpu)); cpu->cycles += 6; break;
        case 0x7E: ROR(cpu, addr_absolute_x(cpu)); cpu->cycles += 7; break;
        
        // BIT
        case 0x24: BIT(cpu, addr_zeropage(cpu)); cpu->cycles += 3; break;
        case 0x2C: BIT(cpu, addr_absolute(cpu)); cpu->cycles += 4; break;
        
        // Branches
        case 0x90: branch(cpu, !GET_FLAG(cpu, FLAG_C)); cpu->cycles += 2; break; // BCC
        case 0xB0: branch(cpu, GET_FLAG(cpu, FLAG_C)); cpu->cycles += 2; break;  // BCS
        case 0xF0: branch(cpu, GET_FLAG(cpu, FLAG_Z)); cpu->cycles += 2; break;  // BEQ
        case 0xD0: branch(cpu, !GET_FLAG(cpu, FLAG_Z)); cpu->cycles += 2; break; // BNE
        case 0x30: branch(cpu, GET_FLAG(cpu, FLAG_N)); cpu->cycles += 2; break;  // BMI
        case 0x10: branch(cpu, !GET_FLAG(cpu, FLAG_N)); cpu->cycles += 2; break; // BPL
        case 0x50: branch(cpu, !GET_FLAG(cpu, FLAG_V)); cpu->cycles += 2; break; // BVC
        case 0x70: branch(cpu, GET_FLAG(cpu, FLAG_V)); cpu->cycles += 2; break;  // BVS
        
        // Transfers
        case 0xAA: cpu->X = cpu->A; SET_ZN(cpu, cpu->X); cpu->cycles += 2; break; // TAX
        case 0xA8: cpu->Y = cpu->A; SET_ZN(cpu, cpu->Y); cpu->cycles += 2; break; // TAY
        case 0x8A: cpu->A = cpu->X; SET_ZN(cpu, cpu->A); cpu->cycles += 2; break; // TXA
        case 0x98: cpu->A = cpu->Y; SET_ZN(cpu, cpu->A); cpu->cycles += 2; break; // TYA
        case 0xBA: cpu->X = cpu->SP; SET_ZN(cpu, cpu->X); cpu->cycles += 2; break; // TSX
        case 0x9A: cpu->SP = cpu->X; cpu->cycles += 2; break; // TXS
        
        // Stack
        case 0x48: PUSH(cpu, cpu->A); cpu->cycles += 3; break; // PHA
        case 0x68: cpu->A = PULL(cpu); SET_ZN(cpu, cpu->A); cpu->cycles += 4; break; // PLA
        case 0x08: PUSH(cpu, cpu->status | FLAG_B | FLAG_U); cpu->cycles += 3; break; // PHP
        case 0x28: cpu->status = PULL(cpu) | FLAG_U; cpu->cycles += 4; break; // PLP
        
        // Increments/Decrements
        case 0xE8: cpu->X++; SET_ZN(cpu, cpu->X); cpu->cycles += 2; break; // INX
        case 0xC8: cpu->Y++; SET_ZN(cpu, cpu->Y); cpu->cycles += 2; break; // INY
        case 0xCA: cpu->X--; SET_ZN(cpu, cpu->X); cpu->cycles += 2; break; // DEX
        case 0x88: cpu->Y--; SET_ZN(cpu, cpu->Y); cpu->cycles += 2; break; // DEY
        
        // Flags
        case 0x18: CLR_FLAG(cpu, FLAG_C); cpu->cycles += 2; break; // CLC
        case 0x38: SET_FLAG(cpu, FLAG_C); cpu->cycles += 2; break; // SEC
        case 0x58: CLR_FLAG(cpu, FLAG_I); cpu->cycles += 2; break; // CLI
        case 0x78: SET_FLAG(cpu, FLAG_I); cpu->cycles += 2; break; // SEI
        case 0xB8: CLR_FLAG(cpu, FLAG_V); cpu->cycles += 2; break; // CLV
        case 0xD8: CLR_FLAG(cpu, FLAG_D); cpu->cycles += 2; break; // CLD
        case 0xF8: SET_FLAG(cpu, FLAG_D); cpu->cycles += 2; break; // SED
        
        // Jump/Call
        case 0x4C: cpu->PC = memory_read_word(cpu->PC); cpu->cycles += 3; break; // JMP abs
        case 0x6C: { uint16_t addr = memory_read_word(cpu->PC);
                     cpu->PC = memory_read(addr) | (memory_read((addr & 0xFF00) | ((addr + 1) & 0xFF)) << 8);
                     cpu->cycles += 5; } break; // JMP ind
        case 0x20: { uint16_t addr = memory_read_word(cpu->PC);
                     cpu->PC += 1;
                     PUSH(cpu, (cpu->PC >> 8) & 0xFF);
                     PUSH(cpu, cpu->PC & 0xFF);
                     cpu->PC = addr;
                     cpu->cycles += 6; } break; // JSR
        case 0x60: { uint8_t lo = PULL(cpu);
                     uint8_t hi = PULL(cpu);
                     cpu->PC = (hi << 8) | lo;
                     cpu->PC++;
                     cpu->cycles += 6; } break; // RTS
        case 0x40: { cpu->status = PULL(cpu) | FLAG_U;
                     uint8_t lo = PULL(cpu);
                     uint8_t hi = PULL(cpu);
                     cpu->PC = (hi << 8) | lo;
                     cpu->cycles += 6; } break; // RTI
        
        // System
        case 0x00: cpu->PC++; PUSH(cpu, (cpu->PC >> 8) & 0xFF); PUSH(cpu, cpu->PC & 0xFF);
                   PUSH(cpu, cpu->status | FLAG_B | FLAG_U);
                   SET_FLAG(cpu, FLAG_I);
                   cpu->PC = memory_read_word(0xFFFE);
                   cpu->cycles += 7; break; // BRK
        case 0xEA: cpu->cycles += 2; break; // NOP
        
        default:
            printf("Unknown opcode: 0x%02X at PC=0x%04X\n", opcode, cpu->PC - 1);
            break;
    }
}

void cpu_execute(CPU *cpu, uint64_t max_cycles) {
    uint64_t start_cycles = cpu->cycles;
    while (cpu->cycles - start_cycles < max_cycles) {
        cpu_step(cpu);
    }
}
