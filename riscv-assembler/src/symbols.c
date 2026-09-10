#include <stdio.h>
#include <string.h>
#include "symbols.h"

/* ----------------------------------------------------------------
   symbols_build — first pass over the program
   Scans every line looking for labels and records their addresses
   ---------------------------------------------------------------- */
int symbols_build(const Program *program, SymbolTable *table) {

    table->num_symbols = 0;

    /* Each instruction is 4 bytes (32 bits)
       PC starts at 0 and increments by 4 for each instruction */
    int current_address = 0;

    for (int i = 0; i < program->num_lines; i++) {
        const Line *line = &program->lines[i];

        /* If this line has a label, record it in the table */
        if (line->is_label) {

            /* Check we don't exceed the maximum number of symbols */
            if (table->num_symbols >= MAX_SYMBOLS) {
                fprintf(stderr, "Error: too many labels (max %d)\n", MAX_SYMBOLS);
                return -1;
            }

            /* Check for duplicate labels */
            if (symbols_lookup(table, line->label) != -1) {
                fprintf(stderr, "Error: duplicate label '%s' at line %d\n",
                        line->label, line->line_number);
                return -1;
            }

            /* Add the label to the table */
            Symbol *sym = &table->symbols[table->num_symbols];
            strcpy(sym->name, line->label);
            sym->address = current_address;
            table->num_symbols++;
        }

        /* If this line has instructions, advance the PC by 4 bytes */
        if (line->num_tokens > 0) {
            current_address += 4;
        }
    }

    return 0; /* success */
}

/* ----------------------------------------------------------------
   symbols_lookup — searches for a label in the symbol table
   Returns the byte address if found, -1 if not found
   ---------------------------------------------------------------- */
int symbols_lookup(const SymbolTable *table, const char *name) {

    for (int i = 0; i < table->num_symbols; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return table->symbols[i].address;
        }
    }

    return -1; /* not found */
}

/* ----------------------------------------------------------------
   symbols_print — prints the symbol table (for debugging)
   ---------------------------------------------------------------- */
void symbols_print(const SymbolTable *table) {

    printf("=== Symbol Table (%d labels) ===\n", table->num_symbols);

    for (int i = 0; i < table->num_symbols; i++) {
        printf("  %-20s → 0x%04X (%d)\n",
               table->symbols[i].name,
               table->symbols[i].address,
               table->symbols[i].address);
    }
}