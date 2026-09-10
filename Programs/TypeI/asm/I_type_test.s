# ===================================================
# Test complet Type I — couvre les 9 operations arithmetiques
# Operandes fixes : x1 = 12
# ===================================================

addi x1, x0, 12       # x1 = 12  (0000...1100)

addi x2, x1, 8         # x2 = 12 + 8   = 20
andi x3, x1, 10        # x3 = 12 & 10  = 8      (1100 & 1010 = 1000)
ori  x4, x1, 3         # x4 = 12 | 3   = 15     (1100 | 0011 = 1111)
xori x5, x1, 10        # x5 = 12 ^ 10  = 6      (1100 ^ 1010 = 0110)
slli x6, x1, 2         # x6 = 12 << 2  = 48
srli x7, x1, 2         # x7 = 12 >> 2 (logique) = 3
srai x8, x1, 2         # x8 = 12 >> 2 (arithm.) = 3   (x1 positif)
slti x9, x1, 20        # x9 = (12 < 20) signe   = 1
sltiu x10, x1, 20      # x10 = (12 < 20) non-signe = 1

# --- Cas limite pour SLTI/SLTIU : distinguer signe vs non-signe ---
addi x11, x0, -1       # x11 = 0xFFFFFFFF (=-1 signe, =4294967295 non-signe)
slti  x12, x11, 1      # x12 = (-1 < 1) signe        = 1  (vrai en signe)
sltiu x13, x11, 1      # x13 = (0xFFFFFFFF < 1) non-signe = 0 (faux en non-signe)