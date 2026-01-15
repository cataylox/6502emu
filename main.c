#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "cpu.h"
#include "memory.h"

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  --load FILE       Load binary FILE into emulator memory\n");
    printf("  --offset OFFSET   Load file at memory OFFSET (hex or decimal)\n");
    printf("                    Default: 0x0000\n");
    printf("                    Example: --offset 0x2000 or --offset 8192\n");
    printf("  --help            Display this help message\n");
    printf("\nExamples:\n");
    printf("  %s --load program.bin\n", program_name);
    printf("  %s --load program.bin --offset 0x2000\n", program_name);
    printf("  %s (runs built-in test program)\n", program_name);
}

int parse_offset(const char *str, uint16_t *offset) {
    char *endptr;
    long value;
    
    // Try parsing as hex (with or without 0x prefix)
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        value = strtol(str, &endptr, 16);
    } else {
        // Try parsing as decimal first
        value = strtol(str, &endptr, 10);
    }
    
    if (*endptr != '\0' || endptr == str) {
        return 0; // Parse error
    }
    
    if (value < 0 || value > 0xFFFF) {
        return 0; // Out of range
    }
    
    *offset = (uint16_t)value;
    return 1;
}

int load_binary_file(const char *filename, uint16_t offset) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s': %s\n", filename, strerror(errno));
        return 0;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) {
        fprintf(stderr, "Error: Cannot determine file size for '%s'\n", filename);
        fclose(f);
        return 0;
    }
    
    if (size == 0) {
        fprintf(stderr, "Error: File '%s' is empty\n", filename);
        fclose(f);
        return 0;
    }
    
    // Check if file fits in memory at given offset
    if ((uint32_t)offset + (uint32_t)size > 0x10000) {
        fprintf(stderr, "Error: File size (%ld bytes) at offset 0x%04X exceeds memory bounds\n", 
                size, offset);
        fclose(f);
        return 0;
    }
    
    // Read file into memory
    for (long i = 0; i < size; i++) {
        int byte = fgetc(f);
        if (byte == EOF) {
            fprintf(stderr, "Error: Unexpected end of file while reading '%s'\n", filename);
            fclose(f);
            return 0;
        }
        memory_write((uint16_t)(offset + i), (uint8_t)byte);
    }
    
    fclose(f);
    printf("Loaded %ld bytes from '%s' at address 0x%04X\n", size, filename, offset);
    return 1;
}

void run_default_program(CPU *cpu) {
    // Example program: Add two numbers
    memory_write(0x0000, 0xA9); // LDA #$05
    memory_write(0x0001, 0x05);
    memory_write(0x0002, 0x69); // ADC #$03
    memory_write(0x0003, 0x03);
    memory_write(0x0004, 0x85); // STA $10
    memory_write(0x0005, 0x10);
    memory_write(0x0006, 0x00); // BRK
    
    cpu->PC = 0x0000;
    
    printf("Running built-in test program...\n");
    printf("Initial state:\n");
    printf("PC: 0x%04X  A: 0x%02X  X: 0x%02X  Y: 0x%02X  SP: 0x%02X  Status: 0x%02X\n",
           cpu->PC, cpu->A, cpu->X, cpu->Y, cpu->SP, cpu->status);
    
    // Execute program
    for (int i = 0; i < 10 && cpu->PC < 0x0007; i++) {
        cpu_step(cpu);
        printf("PC: 0x%04X  A: 0x%02X  X: 0x%02X  Y: 0x%02X  SP: 0x%02X  Status: 0x%02X  Cycles: %lu\n",
               cpu->PC, cpu->A, cpu->X, cpu->Y, cpu->SP, cpu->status, cpu->cycles);
    }
    
    printf("\nResult at $10: 0x%02X (should be 0x08)\n", memory_read(0x10));
}

int main(int argc, char *argv[]) {
    CPU cpu;
    const char *load_file = NULL;
    uint16_t offset = 0x0000;
    int offset_specified = 0;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--load") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --load requires a filename argument\n");
                print_usage(argv[0]);
                return 1;
            }
            load_file = argv[++i];
        } else if (strcmp(argv[i], "--offset") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --offset requires an address argument\n");
                print_usage(argv[0]);
                return 1;
            }
            if (!parse_offset(argv[++i], &offset)) {
                fprintf(stderr, "Error: Invalid offset '%s' (must be 0x0000-0xFFFF)\n", argv[i]);
                return 1;
            }
            offset_specified = 1;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    // Initialize emulator
    memory_init();
    cpu_init(&cpu);
    
    if (load_file) {
        // Load binary file
        if (!load_binary_file(load_file, offset)) {
            return 1;
        }
        
        // Set PC to start execution at the load offset
        cpu.PC = offset;
        printf("Starting execution at address 0x%04X\n", cpu.PC);
        
        // Execute program
        printf("\nInitial state:\n");
        printf("PC: 0x%04X  A: 0x%02X  X: 0x%02X  Y: 0x%02X  SP: 0x%02X  Status: 0x%02X\n",
               cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.status);
        
        // Run for a reasonable number of instructions (or until BRK)
        for (int i = 0; i < 1000; i++) {
            uint8_t opcode = memory_read(cpu.PC);
            cpu_step(&cpu);
            printf("PC: 0x%04X  A: 0x%02X  X: 0x%02X  Y: 0x%02X  SP: 0x%02X  Status: 0x%02X  Cycles: %lu\n",
                   cpu.PC, cpu.A, cpu.X, cpu.Y, cpu.SP, cpu.status, cpu.cycles);
            
            // Stop on BRK instruction (0x00)
            if (opcode == 0x00) {
                printf("\nProgram terminated (BRK instruction)\n");
                break;
            }
        }
    } else {
        // Run default test program
        if (offset_specified) {
            fprintf(stderr, "Warning: --offset specified without --load, ignoring offset\n");
        }
        run_default_program(&cpu);
    }
    
    return 0;
}
