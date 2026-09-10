#ifndef LEXER_H
#define LEXER_H

/* ================================================================
   lexer.h — Tokenizer for the RISC-V RV32IM assembler
   ================================================================
   Role: read the .s file line by line and split each line
         into tokens (pieces)

   Example:
     "addi x1, x0, 3   # load 3"
     → tokens[0] = "addi"
     → tokens[1] = "x1"
     → tokens[2] = "x0"
     → tokens[3] = "3"
     → num_tokens = 4
   ================================================================ */

/* ----------------------------------------------------------------
   CONSTANTS
   ---------------------------------------------------------------- */

#define MAX_TOKENS    10   /* maximum number of tokens per line   */
#define MAX_TOKEN_LEN 32   /* maximum number of characters per token */
#define MAX_LINES     256  /* maximum number of lines in the file */

/* ----------------------------------------------------------------
   STRUCTURES
   ---------------------------------------------------------------- */

/* Represents one line of the .s file after tokenization */
typedef struct {
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN]; /* array of tokens */
    int  num_tokens;                         /* number of tokens found */
    int  line_number;                        /* line number (for error messages) */
    int  is_label;                           /* 1 if the line contains a label */
    char label[MAX_TOKEN_LEN];               /* label name if is_label == 1 */
} Line;

/* Represents the entire file after tokenization */
typedef struct {
    Line lines[MAX_LINES]; /* array of all lines */
    int  num_lines;        /* number of lines found */
} Program;

/* ----------------------------------------------------------------
   FUNCTIONS — declared here, defined in lexer.c
   ---------------------------------------------------------------- */

/* Reads the .s file and fills the Program structure
   - filename : path to the .s file
   - program  : pointer to the structure to fill
   - returns  : 0 on success, -1 on error */
int lexer_load(const char *filename, Program *program);

/* Prints the content of a Program (useful for debugging) */
void lexer_print(const Program *program);

#endif /* LEXER_H */