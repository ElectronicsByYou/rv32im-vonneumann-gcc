# ===================================================
# Test toutes positions — 4 offsets byte + 2 offsets halfword
# Base x1 = 300 (mot aligne)
# ===================================================

addi x1, x0, 300      # x1 = adresse de base

# --- Ecriture des 4 octets, valeurs distinctes par position ---
addi x2, x0, 0x11       # position 0
sb   x2, 0(x1)
addi x3, x0, 0x22       # position 1
sb   x3, 1(x1)
addi x4, x0, 0x33       # position 2
sb   x4, 2(x1)
addi x5, x0, 0x44       # position 3
sb   x5, 3(x1)

# --- Lecture individuelle de chaque position (non-signe pour simplicite) ---
lbu  x6, 0(x1)          # attendu 0x11 = 17
lbu  x7, 1(x1)          # attendu 0x22 = 34
lbu  x8, 2(x1)           # attendu 0x33 = 51
lbu  x9, 3(x1)           # attendu 0x44 = 68

# --- Diagnostic : mot complet, verifie l'ordre exact ---
lw   x10, 0(x1)          # attendu 0x44332211 = 1144201745

# --- Test halfword : positions 0 et 2 dans un autre mot ---
addi x11, x0, 500         # position 0 (offset 4)
sh   x11, 4(x1)
addi x12, x0, 999         # position 2 (offset 6)
sh   x12, 6(x1)

lhu  x13, 4(x1)            # attendu 500
lhu  x14, 6(x1)            # attendu 999

lw   x15, 4(x1)            # diagnostic : mot complet, attendu 65470964 (0x03E701F4)