# ===================================================
# Diagnostic Branch — incrementation par bitmask
# BEQ=+1, BNE=+2, BLT=+4, BGE=+8, BLTU=+16, BGEU=+32
# x20 final permet de lire en binaire quelles branches ont bien fonctionne
# ===================================================

addi x20, x0, 0

# --- BEQ : bit 0 (+1) ---
addi x1, x0, 5
addi x2, x0, 5
beq  x1, x2, beq_ok
jal  x0, beq_fail
beq_ok:
    addi x20, x20, 1
beq_fail:

# --- BNE : bit 1 (+2) ---
addi x3, x0, 5
addi x4, x0, 8
bne  x3, x4, bne_ok
jal  x0, bne_fail
bne_ok:
    addi x20, x20, 2
bne_fail:

# --- BLT : bit 2 (+4) ---
addi x5, x0, -1
addi x6, x0, 1
blt  x5, x6, blt_ok
jal  x0, blt_fail
blt_ok:
    addi x20, x20, 4
blt_fail:

# --- BGE : bit 3 (+8) ---
addi x7, x0, 1
addi x8, x0, -1
bge  x7, x8, bge_ok
jal  x0, bge_fail
bge_ok:
    addi x20, x20, 8
bge_fail:

# --- BLTU : bit 4 (+16) ---
addi x9, x0, 1
addi x10, x0, -1
bltu x9, x10, bltu_ok
jal  x0, bltu_fail
bltu_ok:
    addi x20, x20, 16
bltu_fail:

# --- BGEU : bit 5 (+32) ---
addi x11, x0, -1
addi x12, x0, 1
bgeu x11, x12, bgeu_ok
jal  x0, bgeu_fail
bgeu_ok:
    addi x20, x20, 32
bgeu_fail:

end:
    jal x0, end
