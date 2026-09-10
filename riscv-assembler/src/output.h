#ifndef OUTPUT_H
#define OUTPUT_H

/* ================================================================
   output.h — Output file generator for the RISC-V RV32IM assembler
   ================================================================
   Role: write the encoded instructions to a .txt file
         in Logisim Evolution format (v2.0 raw)

   Example output file:
     v2.0 raw
     00300093
     00500113
     002082B3
   ================================================================ */

#include <stdint.h>  /* uint32_t */

/* ----------------------------------------------------------------
   FUNCTIONS
   ---------------------------------------------------------------- */

/* Opens the output file and writes the Logisim header "v2.0 raw"
   - filename : path to the output .txt file
   - returns  : 0 on success, -1 on error */
int output_open(const char *filename);

/* Writes one 32-bit instruction as an 8-character hex line
   - instr   : the 32-bit encoded instruction
   - returns : 0 on success, -1 on error */
int output_write(uint32_t instr);

/* Closes the output file */
void output_close(void);

#endif /* OUTPUT_H */