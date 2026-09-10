# ===================================================
# Test LUI / AUIPC
# ===================================================

# --- LUI : charge une grande constante ---
lui  x10, 0x12345         # x10 = 0x12345000
addi x10, x10, 0x678       # x10 = 0x12345678 (pattern LUI+ADDI classique)

# --- LUI seul, verification des bits bas a zero ---
lui  x11, 0xFFFFF           # x11 = 0xFFFFF000 (bits bas forces a 0)

# --- AUIPC : adresse relative au PC ---
auipc x12, 0                # x12 = PC_old de cette instruction (adresse courante)
