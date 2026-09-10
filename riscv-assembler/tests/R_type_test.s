# ===================================================
# Test complet Type R — couvre les 10 operations
# Operandes fixes : x1 = 12, x2 = 8
# ===================================================

addi x1, x0, 12      # x1 = 12  (0000...1100)
addi x2, x0, 8       # x2 = 8   (0000...1000)

add  x3, x1, x2      # x3 = 12 + 8   = 20
sub  x4, x1, x2      # x4 = 12 - 8   = 4
and  x5, x1, x2      # x5 = 12 & 8   = 8      (1100 & 1000 = 1000)
or   x6, x1, x2      # x6 = 12 | 8   = 12     (1100 | 1000 = 1100)
xor  x7, x1, x2      # x7 = 12 ^ 8   = 4      (1100 ^ 1000 = 0100)
sll  x8, x1, x2      # x8 = 12 << (8&0x1F) = 12 << 8 = 3072
srl  x9, x1, x2      # x9 = 12 >> 8 (logique) = 0
sra  x10, x1, x2     # x10 = 12 >> 8 (arithm.) = 0  (x1 positif)
slt  x11, x2, x1     # x11 = (8 < 12) signe   = 1
sltu x12, x2, x1     # x12 = (8 < 12) non-signe = 1

# --- Cas limite pour SLT/SLTU : distinguer signe vs non-signe ---
addi x13, x0, -1     # x13 = 0xFFFFFFFF (=-1 signe, =4294967295 non-signe)
addi x14, x0, 1      # x14 = 1
slt  x15, x13, x14   # x15 = (-1 < 1) signe        = 1  (vrai en signe)
sltu x16, x13, x14   # x16 = (0xFFFFFFFF < 1) non-signe = 0 (faux en non-signe)