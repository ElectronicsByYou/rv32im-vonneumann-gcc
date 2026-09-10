#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "encoder.h"

/* ================================================================
   INTERNAL HELPER FUNCTIONS
   ================================================================ */

/* ----------------------------------------------------------------
   parse_register — converts "x0".."x31" to register number 0..31
   returns -1 if invalid
   ---------------------------------------------------------------- */
static int parse_register(const char *token) {

    /* must start with 'x' */
    if (token[0] != 'x') {
        fprintf(stderr, "Error: invalid register '%s' (must start with x)\n", token);
        return -1;
    }

    /* parse the number after 'x' */
    int reg = atoi(token + 1); /* token+1 skips the 'x' */

    if (reg < 0 || reg > 31) {
        fprintf(stderr, "Error: invalid register '%s' (must be x0..x31)\n", token);
        return -1;
    }

    return reg;
}

/* ----------------------------------------------------------------
   parse_immediate — converts a string to an integer
   handles decimal (3, -1) and hex (0xFF)
   returns the value, sets *ok to 0 on error
   ---------------------------------------------------------------- */
static int parse_immediate(const char *token, int *ok) {
    *ok = 1;
    char *end;
    long val = strtol(token, &end, 0); /* base 0 = auto-detect dec/hex */
    if (*end != '\0') {
        fprintf(stderr, "Error: invalid immediate '%s'\n", token);
        *ok = 0;
        return 0;
    }
    return (int)val;
}

/* ----------------------------------------------------------------
   LOW-LEVEL BIT ENCODING FUNCTIONS
   One function per instruction format
   ---------------------------------------------------------------- */

/* Format R : funct7 | rs2 | rs1 | funct3 | rd | opcode
   bits:       31:25   24:20 19:15  14:12   11:7   6:0   */
static uint32_t encode_R(int rd, int rs1, int rs2,
                          int funct3, int funct7) {
    uint32_t instr = 0;
    instr |= (0x33);          /* opcode [6:0]  = 0110011 */
    instr |= (rd     & 0x1F) << 7;   /* rd     [11:7]  */
    instr |= (funct3 & 0x07) << 12;  /* funct3 [14:12] */
    instr |= (rs1    & 0x1F) << 15;  /* rs1    [19:15] */
    instr |= (rs2    & 0x1F) << 20;  /* rs2    [24:20] */
    instr |= (funct7 & 0x7F) << 25;  /* funct7 [31:25] */
    return instr;
}

/* Format I : imm[11:0] | rs1 | funct3 | rd | opcode
   bits:       31:20     19:15  14:12   11:7   6:0   */
static uint32_t encode_I(int rd, int rs1, int imm,
                          int funct3, int opcode) {
    uint32_t instr = 0;
    instr |= (opcode  & 0x7F);                /* opcode [6:0]   */
    instr |= (rd      & 0x1F) << 7;           /* rd     [11:7]  */
    instr |= (funct3  & 0x07) << 12;          /* funct3 [14:12] */
    instr |= (rs1     & 0x1F) << 15;          /* rs1    [19:15] */
    instr |= ((imm    & 0xFFF) << 20);        /* imm    [31:20] */
    return instr;
}

/* Format S : imm[11:5] | rs2 | rs1 | funct3 | imm[4:0] | opcode
   bits:       31:25     24:20 19:15  14:12    11:7        6:0   */
static uint32_t encode_S(int rs1, int rs2, int imm, int funct3) {
    uint32_t instr = 0;
    instr |= (0x23);                           /* opcode   [6:0]  */
    instr |= ((imm & 0x1F) << 7);             /* imm[4:0] [11:7] */
    instr |= (funct3 & 0x07) << 12;           /* funct3   [14:12]*/
    instr |= (rs1    & 0x1F) << 15;           /* rs1      [19:15]*/
    instr |= (rs2    & 0x1F) << 20;           /* rs2      [24:20]*/
    instr |= (((imm >> 5) & 0x7F) << 25);     /* imm[11:5][31:25]*/
    return instr;
}

/* Format B : imm[12|10:5] | rs2 | rs1 | funct3 | imm[4:1|11] | opcode
   bits:       31:25        24:20 19:15  14:12    11:7           6:0   */
static uint32_t encode_B(int rs1, int rs2, int imm, int funct3) {
    uint32_t instr = 0;
    instr |= (0x63);                              /* opcode      [6:0]  */
    instr |= (((imm >> 11) & 0x1) << 7);         /* imm[11]     [7]    */
    instr |= (((imm >>  1) & 0xF) << 8);         /* imm[4:1]    [11:8] */
    instr |= (funct3 & 0x07) << 12;              /* funct3      [14:12]*/
    instr |= (rs1    & 0x1F) << 15;              /* rs1         [19:15]*/
    instr |= (rs2    & 0x1F) << 20;              /* rs2         [24:20]*/
    instr |= (((imm >>  5) & 0x3F) << 25);       /* imm[10:5]   [30:25]*/
    instr |= (((imm >> 12) & 0x1)  << 31);       /* imm[12]     [31]   */
    return instr;
}

/* Format U : imm[31:12] | rd | opcode
   bits:       31:12       11:7  6:0   */
static uint32_t encode_U(int rd, int imm, int opcode) {
    uint32_t instr = 0;
    instr |= (opcode & 0x7F);                    /* opcode  [6:0]  */
    instr |= (rd     & 0x1F) << 7;              /* rd      [11:7] */
    instr |= ((imm   & 0xFFFFF) << 12);         /* imm     [31:12]*/
    return instr;
}

/* Format J : imm[20|10:1|11|19:12] | rd | opcode
   bits:       31:12                  11:7  6:0   */
static uint32_t encode_J(int rd, int imm) {
    uint32_t instr = 0;
    instr |= (0x6F);                              /* opcode      [6:0]  */
    instr |= (rd & 0x1F) << 7;                   /* rd          [11:7] */
    instr |= (((imm >> 12) & 0xFF)  << 12);      /* imm[19:12]  [19:12]*/
    instr |= (((imm >> 11) & 0x1)   << 20);      /* imm[11]     [20]   */
    instr |= (((imm >>  1) & 0x3FF) << 21);      /* imm[10:1]   [30:21]*/
    instr |= (((imm >> 20) & 0x1)   << 31);      /* imm[20]     [31]   */
    return instr;
}

/* ================================================================
   INSTRUCTION ENCODING TABLE
   One entry per instruction mnemonic
   ================================================================ */

/* ----------------------------------------------------------------
   encode_line — main encoding function
   Takes a tokenized line and produces a 32-bit instruction
   ---------------------------------------------------------------- */
int encode_line(const Line *line, const SymbolTable *table,
                int address, uint32_t *result) {

    /* skip lines with no tokens (label-only lines) */
    if (line->num_tokens == 0) {
        return 1; /* nothing to encode, not an error */
    }

    const char *mnemonic = line->tokens[0]; /* first token = instruction name */
    int ok;

    /* ============================================================
       TYPE R — register-to-register
       format: instr rd, rs1, rs2
       ============================================================ */

    if (strcmp(mnemonic, "add") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x0, 0x00); /* funct3=000 funct7=0000000 */
        return 0;
    }
    if (strcmp(mnemonic, "sub") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x0, 0x20); /* funct3=000 funct7=0100000 */
        return 0;
    }
    if (strcmp(mnemonic, "and") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x7, 0x00); /* funct3=111 */
        return 0;
    }
    if (strcmp(mnemonic, "or") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x6, 0x00); /* funct3=110 */
        return 0;
    }
    if (strcmp(mnemonic, "xor") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x4, 0x00); /* funct3=100 */
        return 0;
    }
    if (strcmp(mnemonic, "sll") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x1, 0x00); /* funct3=001 */
        return 0;
    }
    if (strcmp(mnemonic, "srl") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x5, 0x00); /* funct3=101 funct7=0000000 */
        return 0;
    }
    if (strcmp(mnemonic, "sra") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x5, 0x20); /* funct3=101 funct7=0100000 */
        return 0;
    }
    if (strcmp(mnemonic, "slt") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x2, 0x00); /* funct3=010 */
        return 0;
    }
    if (strcmp(mnemonic, "sltu") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x3, 0x00); /* funct3=011 */
        return 0;
    }
    /* Extension M */
    if (strcmp(mnemonic, "mul") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int rs2 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || rs2 < 0) return -1;
        *result = encode_R(rd, rs1, rs2, 0x0, 0x01); /* funct7=0000001 */
        return 0;
    }

    /* ============================================================
       TYPE I — immediate arithmetic
       format: instr rd, rs1, imm
       ============================================================ */

    if (strcmp(mnemonic, "addi") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int imm = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x0, 0x13); /* funct3=000 opcode=0010011 */
        return 0;
    }
    if (strcmp(mnemonic, "andi") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int imm = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x7, 0x13); /* funct3=111 */
        return 0;
    }
    if (strcmp(mnemonic, "ori") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int imm = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x6, 0x13); /* funct3=110 */
        return 0;
    }
    if (strcmp(mnemonic, "xori") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int imm = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x4, 0x13); /* funct3=100 */
        return 0;
    }
    if (strcmp(mnemonic, "slti") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int imm = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x2, 0x13); /* funct3=010 */
        return 0;
    }
    if (strcmp(mnemonic, "sltiu") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int rs1 = parse_register(line->tokens[2]);
        int imm = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x3, 0x13); /* funct3=011 */
        return 0;
    }
    if (strcmp(mnemonic, "slli") == 0) {
        int rd    = parse_register(line->tokens[1]);
        int rs1   = parse_register(line->tokens[2]);
        int shamt = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        /* shamt is bits [24:20], funct7=0000000 in bits [31:25] */
        *result = encode_I(rd, rs1, shamt & 0x1F, 0x1, 0x13);
        return 0;
    }
    if (strcmp(mnemonic, "srli") == 0) {
        int rd    = parse_register(line->tokens[1]);
        int rs1   = parse_register(line->tokens[2]);
        int shamt = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, shamt & 0x1F, 0x5, 0x13); /* funct7=0000000 */
        return 0;
    }
    if (strcmp(mnemonic, "srai") == 0) {
        int rd    = parse_register(line->tokens[1]);
        int rs1   = parse_register(line->tokens[2]);
        int shamt = parse_immediate(line->tokens[3], &ok);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        /* funct7=0100000 → add 0x400 to imm field */
        *result = encode_I(rd, rs1, (shamt & 0x1F) | 0x400, 0x5, 0x13);
        return 0;
    }

    /* ============================================================
       TYPE I — LOAD
       format: instr rd, imm(rs1)
       ============================================================ */

    if (strcmp(mnemonic, "lw") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x2, 0x03); /* funct3=010 opcode=0000011 */
        return 0;
    }
    if (strcmp(mnemonic, "lh") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x1, 0x03); /* funct3=001 */
        return 0;
    }
    if (strcmp(mnemonic, "lb") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x0, 0x03); /* funct3=000 */
        return 0;
    }
    if (strcmp(mnemonic, "lhu") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x5, 0x03); /* funct3=101 */
        return 0;
    }
    if (strcmp(mnemonic, "lbu") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rd < 0 || rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x4, 0x03); /* funct3=100 */
        return 0;
    }

    /* ============================================================
       JALR — format I
       format: jalr rd, imm(rs1)
       ============================================================ */

    if (strcmp(mnemonic, "jalr") == 0) {
        int rd  = parse_register(line->tokens[1]);
        if (rd < 0) return -1;

        /* detect format: jalr rd, rs1, imm  OR  jalr rd, imm(rs1) */
        int rs1, imm;
        if (line->tokens[2][0] == 'x') {
            /* format: jalr rd, rs1, imm */
            rs1 = parse_register(line->tokens[2]);
            imm = parse_immediate(line->tokens[3], &ok);
        } else {
            /* format: jalr rd, imm(rs1) */
            imm = parse_immediate(line->tokens[2], &ok);
            rs1 = parse_register(line->tokens[3]);
        }
        if (rs1 < 0 || !ok) return -1;
        *result = encode_I(rd, rs1, imm, 0x0, 0x67);
        return 0;
    }

    /* ============================================================
       TYPE S — STORE
       format: instr rs2, imm(rs1)
       ============================================================ */

    if (strcmp(mnemonic, "sw") == 0) {
        int rs2 = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rs1 < 0 || rs2 < 0 || !ok) return -1;
        *result = encode_S(rs1, rs2, imm, 0x2); /* funct3=010 */
        return 0;
    }
    if (strcmp(mnemonic, "sh") == 0) {
        int rs2 = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rs1 < 0 || rs2 < 0 || !ok) return -1;
        *result = encode_S(rs1, rs2, imm, 0x1); /* funct3=001 */
        return 0;
    }
    if (strcmp(mnemonic, "sb") == 0) {
        int rs2 = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        int rs1 = parse_register(line->tokens[3]);
        if (rs1 < 0 || rs2 < 0 || !ok) return -1;
        *result = encode_S(rs1, rs2, imm, 0x0); /* funct3=000 */
        return 0;
    }

    /* ============================================================
       TYPE B — BRANCH
       format: instr rs1, rs2, label
       offset = label_address - current_address
       ============================================================ */

    if (strcmp(mnemonic, "beq") == 0  ||
        strcmp(mnemonic, "bne") == 0  ||
        strcmp(mnemonic, "blt") == 0  ||
        strcmp(mnemonic, "bge") == 0  ||
        strcmp(mnemonic, "bltu") == 0 ||
        strcmp(mnemonic, "bgeu") == 0) {

        int rs1 = parse_register(line->tokens[1]);
        int rs2 = parse_register(line->tokens[2]);
        if (rs1 < 0 || rs2 < 0) return -1;

        /* resolve the label to get the target address */
        int target = symbols_lookup(table, line->tokens[3]);
        if (target == -1) {
            fprintf(stderr, "Error: undefined label '%s'\n", line->tokens[3]);
            return -1;
        }

        /* offset = target - current address (signed, in bytes) */
        int offset = target - address;

        /* select funct3 based on mnemonic */
        int funct3 = 0;
        if      (strcmp(mnemonic, "beq")  == 0) funct3 = 0x0;
        else if (strcmp(mnemonic, "bne")  == 0) funct3 = 0x1;
        else if (strcmp(mnemonic, "blt")  == 0) funct3 = 0x4;
        else if (strcmp(mnemonic, "bge")  == 0) funct3 = 0x5;
        else if (strcmp(mnemonic, "bltu") == 0) funct3 = 0x6;
        else if (strcmp(mnemonic, "bgeu") == 0) funct3 = 0x7;

        *result = encode_B(rs1, rs2, offset, funct3);
        return 0;
    }

    /* ============================================================
       TYPE U — LUI / AUIPC
       format: instr rd, imm
       ============================================================ */

    if (strcmp(mnemonic, "lui") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        if (rd < 0 || !ok) return -1;
        *result = encode_U(rd, imm, 0x37); /* opcode=0110111 */
        return 0;
    }
    if (strcmp(mnemonic, "auipc") == 0) {
        int rd  = parse_register(line->tokens[1]);
        int imm = parse_immediate(line->tokens[2], &ok);
        if (rd < 0 || !ok) return -1;
        *result = encode_U(rd, imm, 0x17); /* opcode=0010111 */
        return 0;
    }

    /* ============================================================
       TYPE J — JAL
       format: jal rd, label
       ============================================================ */

    if (strcmp(mnemonic, "jal") == 0) {
        int rd = parse_register(line->tokens[1]);
        if (rd < 0) return -1;

        /* resolve the label */
        int target = symbols_lookup(table, line->tokens[2]);
        if (target == -1) {
            /* try as immediate (numeric offset) */
            int imm = parse_immediate(line->tokens[2], &ok);
            if (!ok) {
                fprintf(stderr, "Error: undefined label '%s'\n", line->tokens[2]);
                return -1;
            }
            *result = encode_J(rd, imm);
        } else {
            int offset = target - address;
            *result = encode_J(rd, offset);
        }
        return 0;
    }


    /* ============================================================
   CSR INSTRUCTIONS — opcode = 0x73
   format : csr(12) | rs1(5) | funct3(3) | rd(5) | opcode(7)
   ============================================================ */

if (strcmp(mnemonic, "csrrw")  == 0 ||
    strcmp(mnemonic, "csrrs")  == 0 ||
    strcmp(mnemonic, "csrrc")  == 0 ||
    strcmp(mnemonic, "csrrwi") == 0 ||
    strcmp(mnemonic, "csrrsi") == 0 ||
    strcmp(mnemonic, "csrrci") == 0) {

    /* format : csrXX rd, csr, rs1
       tokens[0] = mnemonic
       tokens[1] = rd
       tokens[2] = csr (nom ou valeur hex)
       tokens[3] = rs1 ou imm */

    int rd = parse_register(line->tokens[1]);
    if (rd < 0) return -1;

    /* parse csr address — nom ou valeur */
    int csr = 0;
    if (strcmp(line->tokens[2], "mstatus") == 0) csr = 0x300;
    else if (strcmp(line->tokens[2], "mie")     == 0) csr = 0x304;
    else if (strcmp(line->tokens[2], "mtvec")   == 0) csr = 0x305;
    else if (strcmp(line->tokens[2], "mepc")    == 0) csr = 0x341;
    else if (strcmp(line->tokens[2], "mcause")  == 0) csr = 0x342;
    else {
        csr = (int)strtol(line->tokens[2], NULL, 0);
    }

    int funct3 = 0;
    int rs1_or_imm = 0;

    if (strcmp(mnemonic, "csrrw")  == 0) {
        funct3 = 0x1;
        rs1_or_imm = parse_register(line->tokens[3]);
        if (rs1_or_imm < 0) return -1;
    }
    else if (strcmp(mnemonic, "csrrs")  == 0) {
        funct3 = 0x2;
        rs1_or_imm = parse_register(line->tokens[3]);
        if (rs1_or_imm < 0) return -1;
    }
    else if (strcmp(mnemonic, "csrrc")  == 0) {
        funct3 = 0x3;
        rs1_or_imm = parse_register(line->tokens[3]);
        if (rs1_or_imm < 0) return -1;
    }
    else if (strcmp(mnemonic, "csrrwi") == 0) {
        funct3 = 0x5;
        rs1_or_imm = parse_immediate(line->tokens[3], &ok);
        if (!ok) return -1;
    }
    else if (strcmp(mnemonic, "csrrsi") == 0) {
        funct3 = 0x6;
        rs1_or_imm = parse_immediate(line->tokens[3], &ok);
        if (!ok) return -1;
    }
    else if (strcmp(mnemonic, "csrrci") == 0) {
        funct3 = 0x7;
        rs1_or_imm = parse_immediate(line->tokens[3], &ok);
        if (!ok) return -1;
    }

    uint32_t instr = 0;
    instr |= 0x73;                          /* opcode [6:0]   */
    instr |= (rd          & 0x1F) << 7;     /* rd     [11:7]  */
    instr |= (funct3      & 0x07) << 12;    /* funct3 [14:12] */
    instr |= (rs1_or_imm  & 0x1F) << 15;   /* rs1    [19:15] */
    instr |= (csr         & 0xFFF) << 20;  /* csr    [31:20] */
    *result = instr;
    return 0;
}

/* ============================================================
   MRET — opcode = 0x73, encodage fixe = 0x30200073
   ============================================================ */

if (strcmp(mnemonic, "mret") == 0) {
    *result = 0x30200073;
    return 0;
}


    /* ============================================================
       Unknown instruction
       ============================================================ */
    fprintf(stderr, "Error: unknown instruction '%s' at line %d\n",
            mnemonic, line->line_number);
    return -1;
}