#ifndef ENCODER_H
#define ENCODER_H

/* ================================================================
   encoder.h — Instruction encoder for the RISC-V RV32IM assembler
   ================================================================
   Role: take a tokenized line and encode it into a 32-bit word

   Example:
     tokens = ["addi", "x1", "x0", "3"]
     → 0x00300093

   One function per instruction format (R, I, S, B, U, J)
   ================================================================ */

#include <stdint.h>  /* uint32_t */
#include "lexer.h"
#include "symbols.h"

/* ----------------------------------------------------------------
   FUNCTIONS
   ---------------------------------------------------------------- */

/* Encodes one line into a 32-bit instruction
   - line    : the tokenized line to encode
   - table   : the symbol table (for resolving labels)
   - address : the current byte address of this instruction
   - result  : pointer to store the 32-bit encoded instruction
   - returns : 0 on success, -1 on error */
int encode_line(const Line *line, const SymbolTable *table,
                int address, uint32_t *result);

#endif /* ENCODER_H */