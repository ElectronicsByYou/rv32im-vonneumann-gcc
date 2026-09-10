#include <stdio.h>
#include "output.h"

/* ----------------------------------------------------------------
   Internal file pointer — only visible inside this file
   ---------------------------------------------------------------- */
static FILE *out_file = NULL;

/* ----------------------------------------------------------------
   output_open — opens the file and writes the Logisim header
   ---------------------------------------------------------------- */
int output_open(const char *filename) {

    out_file = fopen(filename, "w"); /* open in write mode */
    if (out_file == NULL) {
        fprintf(stderr, "Error: cannot create output file '%s'\n", filename);
        return -1;
    }

    /* write the Logisim Evolution header */
    fprintf(out_file, "v2.0 raw\n");

    return 0; /* success */
}

/* ----------------------------------------------------------------
   output_write — writes one instruction as hex
   ---------------------------------------------------------------- */
int output_write(uint32_t instr) {

    if (out_file == NULL) {
        fprintf(stderr, "Error: output file is not open\n");
        return -1;
    }

    /* write 8 hex digits lowercase, followed by newline */
    fprintf(out_file, "%08x\n", instr);

    return 0; /* success */
}

/* ----------------------------------------------------------------
   output_close — closes the output file
   ---------------------------------------------------------------- */
void output_close(void) {
    if (out_file != NULL) {
        fclose(out_file);
        out_file = NULL;
    }
}