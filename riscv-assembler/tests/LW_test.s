# ===================================================
# Test LW — Load Word
# ===================================================

addi x1, x0, 100      # x1 = 100 (adresse de base, en RAM section .data)
addi x2, x0, 42        # x2 = 42 (valeur à stocker manuellement en RAM)
sw   x2, 0(x1)         # RAM[100] = 42
lw   x3, 0(x1)         # x3 = RAM[100] = 42