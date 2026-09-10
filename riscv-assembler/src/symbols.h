#ifndef SYMBOLS_H
#define SYMBOLS_H

/* ================================================================
   symbols.h — Symbol table for the RISC-V RV32IM assembler
   ================================================================
   Role: store the mapping between label names and their addresses
         built during the first pass over the tokenized program

   Example:
     "loop:" is at byte address 4  → {"loop", 4}
     "end:"  is at byte address 20 → {"end",  20}

   Usage during second pass:
     "beq x1, x0, end" → look up "end" → address 20
     offset = 20 - current_pc
   ================================================================ */

#include "lexer.h" /* we need the Program structure */

/* ----------------------------------------------------------------
   CONSTANTS
   ---------------------------------------------------------------- */

#define MAX_SYMBOLS    64  /* maximum number of labels in the program */
#define MAX_SYMBOL_LEN 32  /* maximum length of a label name          */

/* ----------------------------------------------------------------
   STRUCTURES
   ---------------------------------------------------------------- */

/* Represents one entry in the symbol table */
typedef struct {
    char name[MAX_SYMBOL_LEN]; /* label name  e.g. "loop" */
    int  address;              /* byte address e.g. 4     */
} Symbol;

/* Represents the entire symbol table */
typedef struct {
    Symbol symbols[MAX_SYMBOLS]; /* array of all symbols */
    int    num_symbols;          /* number of symbols found */
} SymbolTable;

/* ----------------------------------------------------------------
   FUNCTIONS — declared here, defined in symbols.c
   ---------------------------------------------------------------- */

/* First pass — scans the program and builds the symbol table
   - program : the tokenized program from the lexer
   - table   : pointer to the symbol table to fill
   - returns : 0 on success, -1 on error */
int symbols_build(const Program *program, SymbolTable *table);

/* Looks up a label name in the symbol table
   - table : the symbol table to search
   - name  : the label name to look for
   - returns : the byte address if found, -1 if not found */
int symbols_lookup(const SymbolTable *table, const char *name);

/* Prints the symbol table (useful for debugging) */
void symbols_print(const SymbolTable *table);

#endif /* SYMBOLS_H */