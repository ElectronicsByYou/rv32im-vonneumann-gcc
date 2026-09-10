# ===================================================
# Test Branch — les 6 variantes
# x20 = compteur de succes (increment si le branchement se comporte bien)
# ===================================================

addi x20, x0, 0        # x20 = compteur de tests reussis

# --- BEQ : egal ---
addi x1, x0, 5
addi x2, x0, 5
beq  x1, x2, beq_ok
jal  x0, beq_fail
beq_ok:
    addi x20, x20, 1    # +1 si BEQ pris correctement
beq_fail:

# --- BNE : different ---
addi x3, x0, 5
addi x4, x0, 8
bne  x3, x4, bne_ok
jal  x0, bne_fail
bne_ok:
    addi x20, x20, 1
bne_fail:

# --- BLT : inferieur signe (-1 < 1) ---
addi x5, x0, -1
addi x6, x0, 1
blt  x5, x6, blt_ok
jal  x0, blt_fail
blt_ok:
    addi x20, x20, 1
blt_fail:

# --- BGE : superieur ou egal signe (1 >= -1) ---
addi x7, x0, 1
addi x8, x0, -1
bge  x7, x8, bge_ok
jal  x0, bge_fail
bge_ok:
    addi x20, x20, 1
bge_fail:

# --- BLTU : inferieur non-signe (1 < 0xFFFFFFFF) ---
addi x9, x0, 1
addi x10, x0, -1        # 0xFFFFFFFF en non-signe = tres grand
bltu x9, x10, bltu_ok
jal  x0, bltu_fail
bltu_ok:
    addi x20, x20, 1
bltu_fail:

# --- BGEU : superieur ou egal non-signe (0xFFFFFFFF >= 1) ---
addi x11, x0, -1
addi x12, x0, 1
bgeu x11, x12, bgeu_ok
jal  x0, bgeu_fail
bgeu_ok:
    addi x20, x20, 1
bgeu_fail:

end:
    jal x0, end