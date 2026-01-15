#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "basic.h"

const char *test_program = 
"10 PRINT \"6502 BASIC INTERPRETER\"\n"
"20 PRINT \"======================\"\n"
"30 PRINT\n"
"40 LET A = 10\n"
"50 LET B = 20\n"
"60 PRINT \"A = \"; A\n"
"70 PRINT \"B = \"; B\n"
"80 LET C = A + B\n"
"90 PRINT \"A + B = \"; C\n"
"100 PRINT\n"
"110 PRINT \"Counting from 1 to 5:\"\n"
"120 FOR I = 1 TO 5\n"
"130 PRINT I\n"
"140 NEXT I\n"
"150 PRINT\n"
"160 PRINT \"Testing conditionals:\"\n"
"170 LET X = 15\n"
"180 IF X > 10 THEN PRINT \"X is greater than 10\"\n"
"190 IF X < 10 THEN PRINT \"X is less than 10\"\n"
"200 PRINT\n"
"210 PRINT \"Fibonacci sequence:\"\n"
"220 LET F = 0\n"
"230 LET G = 1\n"
"240 FOR J = 1 TO 10\n"
"250 PRINT F; \" \";\n"
"260 LET H = F + G\n"
"270 LET F = G\n"
"280 LET G = H\n"
"290 NEXT J\n"
"300 PRINT\n"
"310 PRINT\n"
"320 PRINT \"Done!\"\n"
"330 END\n";

const char *interactive_program =
"10 PRINT \"ENTER YOUR NAME:\"\n"
"20 INPUT N\n"
"30 PRINT \"HELLO USER\"; N\n"
"40 PRINT \"ENTER A NUMBER:\"\n"
"50 INPUT A\n"
"60 PRINT \"ENTER ANOTHER NUMBER:\"\n"
"70 INPUT B\n"
"80 PRINT \"THE SUM IS: \"; A + B\n"
"90 PRINT \"THE PRODUCT IS: \"; A * B\n"
"100 END\n";

char* load_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Error: Could not open file '%s'\n", filename);
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        printf("Error: Out of memory\n");
        fclose(f);
        return NULL;
    }
    
    fread(buffer, 1, size, f);
    buffer[size] = 0;
    fclose(f);
    
    return buffer;
}

void print_menu() {
    printf("\n6502 BASIC INTERPRETER\n");
    printf("======================\n");
    printf("1. Run demo program\n");
    printf("2. Run interactive program\n");
    printf("3. Exit\n");
    printf("\nSelect option: ");
}

int main(int argc, char *argv[]) {
    // If filename provided, run it directly
    if (argc > 1) {
        char *program = load_file(argv[1]);
        if (program) {
            basic_init();
            basic_load_program(program);
            basic_run();
            free(program);
        }
        return 0;
    }
    
    // Otherwise, show menu
    char choice[10];
    
    while (1) {
        print_menu();
        if (!fgets(choice, sizeof(choice), stdin)) break;
        
        switch (choice[0]) {
            case '1':
                printf("\n");
                basic_init();
                basic_load_program(test_program);
                basic_run();
                break;
                
            case '2':
                printf("\n");
                basic_init();
                basic_load_program(interactive_program);
                basic_run();
                break;
                
            case '3':
                printf("Goodbye!\n");
                return 0;
                
            default:
                printf("Invalid option\n");
                break;
        }
    }
    
    return 0;
}
