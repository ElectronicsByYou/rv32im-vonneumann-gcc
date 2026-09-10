#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "symbols.h"
#include "encoder.h"
#include "output.h"

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.s> <output.txt>\n", argv[0]);
        fprintf(stderr, "Example: ./assembler factorial.s factorial.txt\n");
        return 1;
    }

    const char *input_file  = argv[1];
    const char *output_file = argv[2];

    printf("RISC-V Assembler RV32IM\n");
    printf("Input  : %s\n", input_file);
    printf("Output : %s\n\n", output_file);

    /* --------------------------------------------------------
       Step 1 — Tokenize the input file (lexer)
       -------------------------------------------------------- */
    Program program;
    if (lexer_load(input_file, &program) != 0) return 1;
    lexer_print(&program);

    /* --------------------------------------------------------
       Step 2 — Build symbol table (first pass)
       -------------------------------------------------------- */
    SymbolTable table;
    if (symbols_build(&program, &table) != 0) return 1;
    symbols_print(&table);

    /* --------------------------------------------------------
       Step 3 — Open output file
       -------------------------------------------------------- */
    if (output_open(output_file) != 0) return 1;

    /* --------------------------------------------------------
       Step 4 — Encode instructions (second pass)
       -------------------------------------------------------- */
    printf("\n=== Encoded Instructions ===\n");

    int address = 0;
    int errors  = 0;

    for (int i = 0; i < program.num_lines; i++) {
        const Line *line = &program.lines[i];

        /* skip label-only lines (no instruction to encode) */
        if (line->num_tokens == 0) continue;

        uint32_t instr;
        int ret = encode_line(line, &table, address, &instr);

        if (ret == -1) {
            errors++;
            continue; /* keep going to report all errors */
        }
        if (ret == 1) continue; /* nothing to encode */

        /* print to console for debugging */
        printf("0x%04X : %08X  (%s)\n",
               address, instr, line->tokens[0]);

        /* write to output file */
        output_write(instr);

        address += 4;
    }

    /* --------------------------------------------------------
       Step 5 — Close output file
       -------------------------------------------------------- */
    output_close();

    /* --------------------------------------------------------
       Summary
       -------------------------------------------------------- */
    if (errors > 0) {
        fprintf(stderr, "\nAssembly failed with %d error(s)\n", errors);
        return 1;
    }

    printf("\nAssembly successful!\n");
    printf("  %d instructions encoded\n", address / 4);
    printf("  Output written to: %s\n", output_file);

    return 0;
}