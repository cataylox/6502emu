#include "basic.h"
#include "cpu.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PROGRAM_START 0x0800
#define VARIABLES_START 0x0200
#define STACK_START 0x0100
#define MAX_LINE_LEN 256
#define MAX_LINES 256

typedef struct {
    uint16_t line_num;
    char text[MAX_LINE_LEN];
    uint16_t addr;
} BasicLine;

static BasicLine program[MAX_LINES];
static int program_size = 0;
static CPU cpu;
static int32_t variables[26]; // A-Z variables
static uint16_t current_line = 0;
static char input_buffer[MAX_LINE_LEN];

// Token types
typedef enum {
    TOK_NUMBER,
    TOK_VARIABLE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULT,
    TOK_DIV,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EQUALS,
    TOK_LT,
    TOK_GT,
    TOK_LE,
    TOK_GE,
    TOK_NE,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_STRING,
    TOK_EOL,
    TOK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    int32_t value;
    char str[MAX_LINE_LEN];
} Token;

static Token tokens[64];
static int token_count = 0;
static int token_pos = 0;

// Tokenizer
static void skip_spaces(const char **p) {
    while (**p == ' ' || **p == '\t') (*p)++;
}

static int is_keyword(const char *p, const char *keyword) {
    int len = strlen(keyword);
    if (strncmp(p, keyword, len) == 0 && !isalnum(p[len])) {
        return len;
    }
    return 0;
}

static void tokenize(const char *line) {
    token_count = 0;
    const char *p = line;
    
    while (*p && token_count < 64) {
        skip_spaces(&p);
        if (*p == 0) break;
        
        Token *tok = &tokens[token_count++];
        
        if (isdigit(*p)) {
            tok->type = TOK_NUMBER;
            tok->value = atoi(p);
            while (isdigit(*p)) p++;
        } else if (isalpha(*p)) {
            if (isupper(*p) && (!p[1] || !isalnum(p[1]))) {
                tok->type = TOK_VARIABLE;
                tok->value = *p - 'A';
                p++;
            } else {
                tok->type = TOK_UNKNOWN;
                int i = 0;
                while (isalnum(*p) && i < MAX_LINE_LEN - 1) {
                    tok->str[i++] = toupper(*p++);
                }
                tok->str[i] = 0;
            }
        } else if (*p == '"') {
            tok->type = TOK_STRING;
            p++;
            int i = 0;
            while (*p && *p != '"' && i < MAX_LINE_LEN - 1) {
                tok->str[i++] = *p++;
            }
            tok->str[i] = 0;
            if (*p == '"') p++;
        } else if (*p == '+') {
            tok->type = TOK_PLUS;
            p++;
        } else if (*p == '-') {
            tok->type = TOK_MINUS;
            p++;
        } else if (*p == '*') {
            tok->type = TOK_MULT;
            p++;
        } else if (*p == '/') {
            tok->type = TOK_DIV;
            p++;
        } else if (*p == '(') {
            tok->type = TOK_LPAREN;
            p++;
        } else if (*p == ')') {
            tok->type = TOK_RPAREN;
            p++;
        } else if (*p == '=') {
            tok->type = TOK_EQUALS;
            p++;
        } else if (*p == '<') {
            if (p[1] == '=') {
                tok->type = TOK_LE;
                p += 2;
            } else if (p[1] == '>') {
                tok->type = TOK_NE;
                p += 2;
            } else {
                tok->type = TOK_LT;
                p++;
            }
        } else if (*p == '>') {
            if (p[1] == '=') {
                tok->type = TOK_GE;
                p += 2;
            } else {
                tok->type = TOK_GT;
                p++;
            }
        } else if (*p == ',') {
            tok->type = TOK_COMMA;
            p++;
        } else if (*p == ';') {
            tok->type = TOK_SEMICOLON;
            p++;
        } else {
            tok->type = TOK_UNKNOWN;
            tok->str[0] = *p++;
            tok->str[1] = 0;
        }
    }
    
    tokens[token_count].type = TOK_EOL;
}

// Expression evaluator
static int32_t eval_expression();

static int32_t eval_primary() {
    if (token_pos >= token_count) return 0;
    
    Token *tok = &tokens[token_pos];
    
    if (tok->type == TOK_NUMBER) {
        token_pos++;
        return tok->value;
    } else if (tok->type == TOK_VARIABLE) {
        token_pos++;
        return variables[tok->value];
    } else if (tok->type == TOK_LPAREN) {
        token_pos++;
        int32_t val = eval_expression();
        if (token_pos < token_count && tokens[token_pos].type == TOK_RPAREN) {
            token_pos++;
        }
        return val;
    } else if (tok->type == TOK_MINUS) {
        token_pos++;
        return -eval_primary();
    }
    
    return 0;
}

static int32_t eval_term() {
    int32_t val = eval_primary();
    
    while (token_pos < token_count) {
        Token *tok = &tokens[token_pos];
        if (tok->type == TOK_MULT) {
            token_pos++;
            val *= eval_primary();
        } else if (tok->type == TOK_DIV) {
            token_pos++;
            int32_t divisor = eval_primary();
            if (divisor != 0) val /= divisor;
        } else {
            break;
        }
    }
    
    return val;
}

static int32_t eval_expression() {
    int32_t val = eval_term();
    
    while (token_pos < token_count) {
        Token *tok = &tokens[token_pos];
        if (tok->type == TOK_PLUS) {
            token_pos++;
            val += eval_term();
        } else if (tok->type == TOK_MINUS) {
            token_pos++;
            val -= eval_term();
        } else {
            break;
        }
    }
    
    return val;
}

static int eval_condition() {
    int32_t left = eval_expression();
    
    if (token_pos >= token_count) return left != 0;
    
    Token *tok = &tokens[token_pos];
    TokenType op = tok->type;
    
    if (op == TOK_EQUALS || op == TOK_LT || op == TOK_GT || 
        op == TOK_LE || op == TOK_GE || op == TOK_NE) {
        token_pos++;
        int32_t right = eval_expression();
        
        switch (op) {
            case TOK_EQUALS: return left == right;
            case TOK_LT: return left < right;
            case TOK_GT: return left > right;
            case TOK_LE: return left <= right;
            case TOK_GE: return left >= right;
            case TOK_NE: return left != right;
            default: return 0;
        }
    }
    
    return left != 0;
}

// Statement executors
static void exec_print() {
    int newline = 1;
    
    while (token_pos < token_count && tokens[token_pos].type != TOK_EOL) {
        if (tokens[token_pos].type == TOK_STRING) {
            printf("%s", tokens[token_pos].str);
            token_pos++;
            newline = 1;
        } else if (tokens[token_pos].type == TOK_SEMICOLON) {
            token_pos++;
            newline = 0;
        } else if (tokens[token_pos].type == TOK_COMMA) {
            printf("\t");
            token_pos++;
            newline = 1;
        } else {
            printf("%d", eval_expression());
            newline = 1;
        }
    }
    
    if (newline) printf("\n");
}

static void exec_let() {
    if (token_pos >= token_count || tokens[token_pos].type != TOK_VARIABLE) {
        printf("Syntax error in LET\n");
        return;
    }
    
    int var_idx = tokens[token_pos].value;
    token_pos++;
    
    if (token_pos >= token_count || tokens[token_pos].type != TOK_EQUALS) {
        printf("Syntax error: expected =\n");
        return;
    }
    token_pos++;
    
    variables[var_idx] = eval_expression();
}

static void exec_input() {
    while (token_pos < token_count && tokens[token_pos].type != TOK_EOL) {
        if (tokens[token_pos].type == TOK_STRING) {
            printf("%s", tokens[token_pos].str);
            token_pos++;
        } else if (tokens[token_pos].type == TOK_VARIABLE) {
            int var_idx = tokens[token_pos].value;
            token_pos++;
            
            if (fgets(input_buffer, MAX_LINE_LEN, stdin)) {
                variables[var_idx] = atoi(input_buffer);
            }
        } else if (tokens[token_pos].type == TOK_COMMA || 
                   tokens[token_pos].type == TOK_SEMICOLON) {
            token_pos++;
        } else {
            token_pos++;
        }
    }
}

static int exec_goto() {
    int target = eval_expression();
    
    for (int i = 0; i < program_size; i++) {
        if (program[i].line_num == target) {
            current_line = i;
            return 1;
        }
    }
    
    printf("Line %d not found\n", target);
    return 0;
}

static int exec_if() {
    int condition = eval_condition();
    
    // Look for THEN
    if (token_pos < token_count && tokens[token_pos].type == TOK_UNKNOWN &&
        strcmp(tokens[token_pos].str, "THEN") == 0) {
        token_pos++;
    }
    
    if (condition) {
        // Execute the rest of the line
        return 1;
    } else {
        // Skip to next line
        return 0;
    }
}

static void exec_for() {
    if (token_pos >= token_count || tokens[token_pos].type != TOK_VARIABLE) {
        printf("Syntax error in FOR\n");
        return;
    }
    
    int var_idx = tokens[token_pos].value;
    token_pos++;
    
    if (token_pos >= token_count || tokens[token_pos].type != TOK_EQUALS) {
        printf("Syntax error: expected =\n");
        return;
    }
    token_pos++;
    
    variables[var_idx] = eval_expression();
    
    // Skip TO keyword
    if (token_pos < token_count && tokens[token_pos].type == TOK_UNKNOWN &&
        strcmp(tokens[token_pos].str, "TO") == 0) {
        token_pos++;
    }
}

static void exec_next() {
    if (token_pos >= token_count || tokens[token_pos].type != TOK_VARIABLE) {
        printf("Syntax error in NEXT\n");
        return;
    }
    
    int var_idx = tokens[token_pos].value;
    token_pos++;
    
    variables[var_idx]++;
    
    // Find matching FOR
    for (int i = current_line - 1; i >= 0; i--) {
        tokenize(program[i].text);
        token_pos = 0;
        
        if (token_pos < token_count && tokens[token_pos].type == TOK_UNKNOWN &&
            strcmp(tokens[token_pos].str, "FOR") == 0) {
            token_pos++;
            
            if (token_pos < token_count && tokens[token_pos].type == TOK_VARIABLE &&
                tokens[token_pos].value == var_idx) {
                
                // Parse FOR line to get limit
                token_pos++;
                if (token_pos < token_count && tokens[token_pos].type == TOK_EQUALS) {
                    token_pos++;
                    eval_expression(); // Skip initial value
                    
                    if (token_pos < token_count && tokens[token_pos].type == TOK_UNKNOWN &&
                        strcmp(tokens[token_pos].str, "TO") == 0) {
                        token_pos++;
                        int32_t limit = eval_expression();
                        
                        if (variables[var_idx] <= limit) {
                            current_line = i;
                            return;
                        }
                    }
                }
            }
        }
    }
}

static void execute_line(const char *line) {
    tokenize(line);
    token_pos = 0;
    
    while (token_pos < token_count && tokens[token_pos].type != TOK_EOL) {
        if (tokens[token_pos].type == TOK_UNKNOWN) {
            char *cmd = tokens[token_pos].str;
            token_pos++;
            
            if (strcmp(cmd, "PRINT") == 0) {
                exec_print();
            } else if (strcmp(cmd, "LET") == 0) {
                exec_let();
            } else if (strcmp(cmd, "INPUT") == 0) {
                exec_input();
            } else if (strcmp(cmd, "GOTO") == 0) {
                if (exec_goto()) return;
            } else if (strcmp(cmd, "IF") == 0) {
                if (!exec_if()) return;
            } else if (strcmp(cmd, "FOR") == 0) {
                exec_for();
            } else if (strcmp(cmd, "NEXT") == 0) {
                exec_next();
            } else if (strcmp(cmd, "END") == 0) {
                current_line = program_size;
                return;
            } else if (strcmp(cmd, "REM") == 0) {
                return; // Ignore rest of line
            } else {
                printf("Unknown command: %s\n", cmd);
            }
        } else if (tokens[token_pos].type == TOK_VARIABLE) {
            // Implicit LET
            exec_let();
        } else {
            token_pos++;
        }
    }
}

void basic_init() {
    memory_init();
    cpu_init(&cpu);
    program_size = 0;
    memset(variables, 0, sizeof(variables));
}

void basic_load_program(const char *source) {
    char line[MAX_LINE_LEN];
    const char *p = source;
    program_size = 0;
    
    while (*p && program_size < MAX_LINES) {
        // Read one line
        int i = 0;
        while (*p && *p != '\n' && i < MAX_LINE_LEN - 1) {
            line[i++] = *p++;
        }
        line[i] = 0;
        if (*p == '\n') p++;
        
        // Skip empty lines
        if (line[0] == 0) continue;
        
        // Parse line number
        if (isdigit(line[0])) {
            int line_num = atoi(line);
            const char *text = line;
            while (*text && isdigit(*text)) text++;
            while (*text == ' ') text++;
            
            program[program_size].line_num = line_num;
            strncpy(program[program_size].text, text, MAX_LINE_LEN - 1);
            program[program_size].text[MAX_LINE_LEN - 1] = 0;
            program_size++;
        }
    }
}

void basic_run() {
    current_line = 0;
    
    while (current_line < program_size) {
        execute_line(program[current_line].text);
        current_line++;
    }
}
