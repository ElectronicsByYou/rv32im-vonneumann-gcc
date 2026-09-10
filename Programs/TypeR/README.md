# Type R — Test groupé complet (10 opérations)

## 1. Ce que fait chaque instruction (spec RISC-V officielle)

```
ADD  rd, rs1, rs2  : rd ← rs1 + rs2
SUB  rd, rs1, rs2  : rd ← rs1 - rs2
AND  rd, rs1, rs2  : rd ← rs1 & rs2
OR   rd, rs1, rs2  : rd ← rs1 | rs2
XOR  rd, rs1, rs2  : rd ← rs1 ^ rs2
SLL  rd, rs1, rs2  : rd ← rs1 << rs2[4:0]                (décalage logique gauche)
SRL  rd, rs1, rs2  : rd ← rs1 >> rs2[4:0]  (logique)      (remplit de 0)
SRA  rd, rs1, rs2  : rd ← rs1 >> rs2[4:0]  (arithmétique) (propage le bit de signe)
SLT  rd, rs1, rs2  : rd ← (rs1 < rs2) ? 1 : 0             (comparaison signée)
SLTU rd, rs1, rs2  : rd ← (rs1 < rs2) ? 1 : 0             (comparaison non-signée)
```
Spec : *"SLT and SLTU perform signed and unsigned compares respectively, writing 1 to rd if rs1 < rs2, 0 otherwise."* — le résultat écrit est toujours l'entier `1` ou `0`, jamais `-1`.

## 2. Décomposition binaire — Format Type R (commun aux 10)

| Champ | Bits | Rôle |
|---|---|---|
| opcode | `[6:0]` | `0110011` (0x33), fixe pour toutes |
| rd | `[11:7]` | registre destination |
| funct3 | `[14:12]` | distingue l'opération |
| rs1 | `[19:15]` | registre source 1 |
| rs2 | `[24:20]` | registre source 2 |
| funct7 | `[31:25]` | `0000000` sauf SUB/SRA (`0100000`) |

## 3. Code assembleur testé

```asm
addi x1, x0, 12      # x1 = 12
addi x2, x0, 8        # x2 = 8

add  x3, x1, x2       # x3 = 12 + 8   = 20
sub  x4, x1, x2       # x4 = 12 - 8   = 4
and  x5, x1, x2       # x5 = 12 & 8   = 8
or   x6, x1, x2       # x6 = 12 | 8   = 12
xor  x7, x1, x2       # x7 = 12 ^ 8   = 4
sll  x8, x1, x2       # x8 = 12 << 8  = 3072
srl  x9, x1, x2       # x9 = 12 >> 8 (logique)     = 0
sra  x10, x1, x2      # x10 = 12 >> 8 (arithm.)    = 0
slt  x11, x2, x1      # x11 = (8 < 12) signé       = 1
sltu x12, x2, x1      # x12 = (8 < 12) non-signé   = 1

addi x13, x0, -1      # x13 = 0xFFFFFFFF (-1)
addi x14, x0, 1       # x14 = 1
slt  x15, x13, x14    # x15 = (-1 < 1) signé        = 1
sltu x16, x13, x14    # x16 = (0xFFFFFFFF < 1) non-signé = 0
```

## 4. ROM ALUop — table complète (16 × 4 bits)

| Adresse (hex) | `{bit5,funct3}` (bin) | Instructions | Valeur ALUop (hex) | Valeur ALUop (bin) |
|---|---|---|---|---|
| 0 | 0000 | ADD, ADDI | 0 | 0000 |
| 1 | 0001 | SLL, SLLI | 5 | 0101 |
| 2 | 0010 | SLT, SLTI | 8 | 1000 |
| 3 | 0011 | SLTU, SLTIU | 9 | 1001 |
| 4 | 0100 | XOR, XORI | 4 | 0100 |
| 5 | 0101 | SRL, SRLI | 6 | 0110 |
| 6 | 0110 | OR, ORI | 3 | 0011 |
| 7 | 0111 | AND, ANDI | 2 | 0010 |
| 8 | 1000 | SUB | 1 | 0001 |
| 9–C | — | inutilisées | 0 | 0000 |
| **D** | 1101 | **SRA, SRAI** | **7** | **0111** |
| E–F | — | inutilisées | 0 | 0000 |

## 5. Programme complet — état par état

**Instruction 1 : `addi x1, x0, 12`** — addition avec immédiat : `x1 ← x0 + 12`
```
Etat 0 — Fetch     : IR ← 0x00C00093, PC ← 4, PC_old ← 0
Etat 1 — Decode    : SA = X0 = 0, ImmExt = 12 (ImmSrc=000)
Etat 2 — Execute   : ALUout ← SA+ImmExt = 0+12 = 12 (ALUSrc=1, ALUop=0000=ADD)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X1 ← 12, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 2 : `addi x2, x0, 8`** — addition avec immédiat : `x2 ← x0 + 8`
```
Etat 0 — Fetch     : IR ← 0x00800113, PC ← 8, PC_old ← 4
Etat 1 — Decode    : SA = X0 = 0, ImmExt = 8 (ImmSrc=000)
Etat 2 — Execute   : ALUout ← SA+ImmExt = 0+8 = 8 (ALUSrc=1, ALUop=0000=ADD)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X2 ← 8, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 3 : `add x3, x1, x2`** — addition registre-registre : `x3 ← x1 + x2`
```
Etat 0 — Fetch     : IR ← 0x002081B3, PC ← 12, PC_old ← 8
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA+SB = 12+8 = 20 (ALUSrc=0, ALUop=0000=ADD)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X3 ← 20, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 4 : `sub x4, x1, x2`** — soustraction registre-registre : `x4 ← x1 - x2`
```
Etat 0 — Fetch     : IR ← 0x40208233, PC ← 16, PC_old ← 12
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA-SB = 12-8 = 4 (ALUSrc=0, ALUop=0001=SUB, bit5_gated=1)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X4 ← 4, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 5 : `and x5, x1, x2`** — ET logique bit à bit : `x5 ← x1 & x2`
```
Etat 0 — Fetch     : IR ← 0x0020F2B3, PC ← 20, PC_old ← 16
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA&SB = 12&8 = 8 (ALUSrc=0, ALUop=0010=AND)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X5 ← 8, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 6 : `or x6, x1, x2`** — OU logique bit à bit : `x6 ← x1 | x2`
```
Etat 0 — Fetch     : IR ← 0x0020E333, PC ← 24, PC_old ← 20
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA|SB = 12|8 = 12 (ALUSrc=0, ALUop=0011=OR)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X6 ← 12, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 7 : `xor x7, x1, x2`** — OU exclusif bit à bit : `x7 ← x1 ^ x2`
```
Etat 0 — Fetch     : IR ← 0x0020C3B3, PC ← 28, PC_old ← 24
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA^SB = 12^8 = 4 (ALUSrc=0, ALUop=0100=XOR)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X7 ← 4, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 8 : `sll x8, x1, x2`** — décalage logique à gauche : `x8 ← x1 << x2[4:0]`
```
Etat 0 — Fetch     : IR ← 0x00209433, PC ← 32, PC_old ← 28
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA<<(SB&0x1F) = 12<<8 = 3072 (ALUSrc=0, ALUop=0101=SLL)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X8 ← 3072, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 9 : `srl x9, x1, x2`** — décalage logique à droite (remplit de 0) : `x9 ← x1 >> x2[4:0]`
```
Etat 0 — Fetch     : IR ← 0x0020D4B3, PC ← 36, PC_old ← 32
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA>>(SB&0x1F) logique = 12>>8 = 0 (ALUSrc=0, ALUop=0110=SRL)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X9 ← 0, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 10 : `sra x10, x1, x2`** — décalage arithmétique à droite (propage le signe) : `x10 ← x1 >>> x2[4:0]`
```
Etat 0 — Fetch     : IR ← 0x4020D533, PC ← 40, PC_old ← 36
Etat 1 — Decode    : SA = X1 = 12, SB = X2 = 8
Etat 2 — Execute   : ALUout ← SA>>(SB&0x1F) arithm. = 12>>8 = 0 (ALUSrc=0, ALUop=0111=SRA, bit5_gated=1)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X10 ← 0, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 11 : `slt x11, x2, x1`** — comparaison signée « inférieur strict » : `x11 ← (x2 < x1) ? 1 : 0`
```
Etat 0 — Fetch     : IR ← 0x001125B3, PC ← 44, PC_old ← 40
Etat 1 — Decode    : SA = X2 = 8, SB = X1 = 12
Etat 2 — Execute   : ALUout ← (SA<SB, signé) = (8<12) = 1 (ALUSrc=0, ALUop=1000=SLT)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X11 ← 1, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 12 : `sltu x12, x2, x1`** — comparaison non-signée « inférieur strict » : `x12 ← (x2 < x1) ? 1 : 0`
```
Etat 0 — Fetch     : IR ← 0x00113633, PC ← 48, PC_old ← 44
Etat 1 — Decode    : SA = X2 = 8, SB = X1 = 12
Etat 2 — Execute   : ALUout ← (SA<SB, non-signé) = (8<12) = 1 (ALUSrc=0, ALUop=1001=SLTU)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X12 ← 1, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 13 : `addi x13, x0, -1`** — addition avec immédiat négatif : `x13 ← x0 + (-1)`
```
Etat 0 — Fetch     : IR ← 0xFFF00693, PC ← 52, PC_old ← 48
Etat 1 — Decode    : SA = X0 = 0, ImmExt = -1 = 0xFFFFFFFF (ImmSrc=000)
Etat 2 — Execute   : ALUout ← SA+ImmExt = 0+(-1) = -1 = 0xFFFFFFFF (ALUSrc=1, ALUop=0000=ADD)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X13 ← 0xFFFFFFFF, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 14 : `addi x14, x0, 1`** — addition avec immédiat : `x14 ← x0 + 1`
```
Etat 0 — Fetch     : IR ← 0x00100713, PC ← 56, PC_old ← 52
Etat 1 — Decode    : SA = X0 = 0, ImmExt = 1 (ImmSrc=000)
Etat 2 — Execute   : ALUout ← SA+ImmExt = 0+1 = 1 (ALUSrc=1, ALUop=0000=ADD)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X14 ← 1, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 15 : `slt x15, x13, x14`** — comparaison signée : `x15 ← (x13 < x14) ? 1 : 0`, cas limite pour distinguer du non-signé
```
Etat 0 — Fetch     : IR ← 0x00E6A7B3, PC ← 60, PC_old ← 56
Etat 1 — Decode    : SA = X13 = 0xFFFFFFFF (-1 signé), SB = X14 = 1
Etat 2 — Execute   : ALUout ← (SA<SB, signé) = (-1<1) = 1 (ALUSrc=0, ALUop=1000=SLT)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X15 ← 1, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

**Instruction 16 : `sltu x16, x13, x14`** — comparaison non-signée : `x16 ← (x13 < x14) ? 1 : 0`, même cas limite en non-signé
```
Etat 0 — Fetch     : IR ← 0x00E6B833, PC ← 64, PC_old ← 60
Etat 1 — Decode    : SA = X13 = 0xFFFFFFFF (4294967295 non-signé), SB = X14 = 1
Etat 2 — Execute   : ALUout ← (SA<SB, non-signé) = (4294967295<1) = 0 (ALUSrc=0, ALUop=1001=SLTU)
Etat 3 — Memory    : rien (traversé à vide)
Etat 4 — Writeback : X16 ← 0, RegWrite=1, RESET_CTR=1 → retour Etat 0
```

80 clics d'horloge au total (5 par instruction × 16).

## 6. Résultat du test

✅ **Validé** — tous les résultats conformes après correction de 2 bugs :

```
X1=12, X2=8, X3=20, X4=4, X5=8, X6=12, X7=4, X8=3072,
X9=0, X10=0, X11=1, X12=1, X13=0xFFFFFFFF(-1), X14=1, X15=1, X16=0
```
