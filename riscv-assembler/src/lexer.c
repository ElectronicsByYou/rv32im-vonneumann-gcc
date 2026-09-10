#include <stdio.h>   /* printf, fopen, fgets...  */
#include <string.h>  /* strlen, strcpy, strcmp... */
#include <ctype.h>   /* isspace, isdigit...       */
#include "lexer.h"   /* our own definitions       */

/* ----------------------------------------------------------------
   Internal function (not visible from other files)
   Tokenizes a single raw line and fills a Line structure
   ---------------------------------------------------------------- */
static void tokenize_line(const char *raw_line, Line *line) {

    line->num_tokens = 0;
    line->is_label   = 0;
    line->label[0]   = '\0';

    int i   = 0;
    int len = strlen(raw_line);

    while (i < len) {

        /* skip spaces and tabs */
        if (isspace(raw_line[i])) {
            i++;
            continue;
        }

        /* stop at comment */
        if (raw_line[i] == '#') break;

        /* skip commas */
        if (raw_line[i] == ',') {
            i++;
            continue;
        }

        /* read a token */
        int  j = 0;
        char token[MAX_TOKEN_LEN];

        while (i < len
               && !isspace(raw_line[i])
               && raw_line[i] != ','
               && raw_line[i] != '#'
               && j < MAX_TOKEN_LEN - 1) {
            token[j++] = raw_line[i++];
        }
        token[j] = '\0';

        if (j == 0) continue;

        /* check for label (ends with ':') */
        if (token[j-1] == ':') {
            token[j-1] = '\0';
            line->is_label = 1;
            strcpy(line->label, token);
            continue;
        }

        /* check for imm(reg) format → split into two tokens */
        if (token[j-1] == ')') {
            int k = 0;
            while (k < j && token[k] != '(') k++;

            if (k < j) {
                /* immediate part before '(' */
                char imm_tok[MAX_TOKEN_LEN];
                strncpy(imm_tok, token, k);
                imm_tok[k] = '\0';

                /* register part between '(' and ')' */
                char reg_tok[MAX_TOKEN_LEN];
                int reg_len = j - k - 2;
                strncpy(reg_tok, token + k + 1, reg_len);
                reg_tok[reg_len] = '\0';

                /* add immediate */
                if (line->num_tokens < MAX_TOKENS) {
                    strcpy(line->tokens[line->num_tokens], imm_tok);
                    line->num_tokens++;
                }
                /* add register */
                if (line->num_tokens < MAX_TOKENS) {
                    strcpy(line->tokens[line->num_tokens], reg_tok);
                    line->num_tokens++;
                }
                continue;
            }
        }

        /* regular token */
        if (line->num_tokens < MAX_TOKENS) {
            strcpy(line->tokens[line->num_tokens], token);
            line->num_tokens++;
        }
    }
}
/* ----------------------------------------------------------------
   lexer_load — reads the .s file and fills the Program structure
   ---------------------------------------------------------------- */
int lexer_load(const char *filename, Program *program) {

    /* Open the file in read mode */
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filename);
        return -1;
    }

    program->num_lines = 0;

    char raw_line[256]; /* buffer to read one raw line */
    int  line_number = 0;

    /* Read the file line by line */
    while (fgets(raw_line, sizeof(raw_line), file) != NULL) {
        line_number++;

        /* Check we don't exceed the maximum number of lines */
        if (program->num_lines >= MAX_LINES) {
            fprintf(stderr, "Error: file exceeds maximum of %d lines\n", MAX_LINES);
            fclose(file);
            return -1;
        }

        /* Tokenize this line */
        Line *line = &program->lines[program->num_lines];
        tokenize_line(raw_line, line);
        line->line_number = line_number;

        /* Skip empty lines (no tokens and no label) */
        if (line->num_tokens == 0 && !line->is_label) {
            continue;
        }

        program->num_lines++;
    }

    fclose(file);
    return 0; /* success */
}

/* ----------------------------------------------------------------
   lexer_print — prints the tokenized program (for debugging)
   ---------------------------------------------------------------- */
void lexer_print(const Program *program) {

    printf("=== Tokenized Program (%d lines) ===\n", program->num_lines);

    for (int i = 0; i < program->num_lines; i++) {
        const Line *line = &program->lines[i];

        printf("Line %3d : ", line->line_number);

        /* Print label if present */
        if (line->is_label) {
            printf("[LABEL: %s] ", line->label);
        }

        /* Print all tokens */
        for (int j = 0; j < line->num_tokens; j++) {
            printf("[%s] ", line->tokens[j]);
        }

        printf("\n");
    }
}