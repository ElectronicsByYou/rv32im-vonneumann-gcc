# BEQ / BNE / BLT / BGE / BLTU / BGEU — Branch

## 1. What these instructions do (official RISC-V spec)

```
BEQ  rs1, rs2, imm : if (rs1 == rs2) PC ← PC + sext(imm)
BNE  rs1, rs2, imm : if (rs1 != rs2) PC ← PC + sext(imm)
BLT  rs1, rs2, imm : if (rs1 <  rs2) PC ← PC + sext(imm)   (signed comparison)
BGE  rs1, rs2, imm : if (rs1 >= rs2) PC ← PC + sext(imm)   (signed comparison)
BLTU rs1, rs2, imm : if (rs1 <  rs2) PC ← PC + sext(imm)   (unsigned comparison)
BGEU rs1, rs2, imm : if (rs1 >= rs2) PC ← PC + sext(imm)   (unsigned comparison)
```
If the condition is false, execution simply continues at `PC+4`.

## 2. Binary decomposition — Type B format

| Field | Bits | Role |
|---|---|---|
| opcode | `[6:0]` | `1100011` (0x63), fixed for all 6 |
| imm[11] | `[7]` | immediate bit |
| funct3 | `[14:12]` | distinguishes the operation |
| rs1 | `[19:15]` | first operand |
| rs2 | `[24:20]` | second operand |
| imm[4:1] | `[11:8]` | immediate bits |
| imm[10:5] | `[30:25]` | immediate bits |
| imm[12] | `[31]` | immediate sign bit |

```
funct3 = 000 → BEQ    funct3 = 100 → BLT
funct3 = 001 → BNE    funct3 = 101 → BGE
                       funct3 = 110 → BLTU
                       funct3 = 111 → BGEU
```

## 3. Tested assembly code

```asm
addi x20, x0, 0        # x20 = validation bitmask counter

# --- BEQ: bit 0 (+1) ---
addi x1, x0, 5
addi x2, x0, 5
beq  x1, x2, beq_ok
jal  x0, beq_fail
beq_ok:
    addi x20, x20, 1
beq_fail:

# --- BNE: bit 1 (+2) ---
addi x3, x0, 5
addi x4, x0, 8
bne  x3, x4, bne_ok
jal  x0, bne_fail
bne_ok:
    addi x20, x20, 2
bne_fail:

# --- BLT: bit 2 (+4) ---
addi x5, x0, -1
addi x6, x0, 1
blt  x5, x6, blt_ok
jal  x0, blt_fail
blt_ok:
    addi x20, x20, 4
blt_fail:

# --- BGE: bit 3 (+8) ---
addi x7, x0, 1
addi x8, x0, -1
bge  x7, x8, bge_ok
jal  x0, bge_fail
bge_ok:
    addi x20, x20, 8
bge_fail:

# --- BLTU: bit 4 (+16) ---
addi x9, x0, 1
addi x10, x0, -1
bltu x9, x10, bltu_ok
jal  x0, bltu_fail
bltu_ok:
    addi x20, x20, 16
bltu_fail:

# --- BGEU: bit 5 (+32) ---
addi x11, x0, -1
addi x12, x0, 1
bgeu x11, x12, bgeu_ok
jal  x0, bgeu_fail
bgeu_ok:
    addi x20, x20, 32
bgeu_fail:

end:
    jal x0, end
```

## 4. Instruction-specific control signals

| Signal | Equation |
|---|---|
| `ImmSrc` | `010` (Type B) |
| `ALUSrc` | 0 (compares `rs1` to `rs2`, not to an immediate) |
| `ALUop` | `raw_branch_op`: SUB for BEQ/BNE, SLT for BLT/BGE, SLTU for BLTU/BGEU |
| `raw_result` | `funct3[2] ? ALUResult : Zero` |
| `branch_taken` | `XOR(raw_result, funct3[0])` |
| `PCWrite` | `AND(isState2, isBranch, branch_taken)` (in addition to the other already-covered cases) |
| `PCSrc` | `001` (PC_old+ImmExt) if branch taken, gated by `isState2` |
| `RESET_CTR` | `AND(isState2, isBranch)` — **3-state** cycle (0→1→2→0), no Memory or Writeback |

## 5. Full program — state by state (BEQ block)

**Instruction 1: `addi x20, x0, 0`** — initializes the validation counter to 0
```
State 0 — Fetch     : IR ← 0x00000A13, PC ← 4, PC_old ← 0
State 1 — Decode    : SA = X0 = 0, ImmExt = 0
State 2 — Execute   : ALUout ← 0+0 = 0 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X20 ← 0, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 2: `addi x1, x0, 5`**
```
State 0 — Fetch     : IR ← 0x00500093, PC ← 8, PC_old ← 4
State 1 — Decode    : SA = X0 = 0, ImmExt = 5
State 2 — Execute   : ALUout ← 0+5 = 5 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X1 ← 5, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 3: `addi x2, x0, 5`**
```
State 0 — Fetch     : IR ← 0x00500113, PC ← 12, PC_old ← 8
State 1 — Decode    : SA = X0 = 0, ImmExt = 5
State 2 — Execute   : ALUout ← 0+5 = 5 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X2 ← 5, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 4: `beq x1, x2, beq_ok`** — branch if equal: `if x1==x2, PC ← PC+8`
```
State 0 — Fetch     : IR ← 0x00208463, PC ← 16, PC_old ← 12
State 1 — Decode    : SA = X1 = 5, SB = X2 = 5, ImmExt = 8 (ImmSrc=010, Type B)
State 2 — Execute   : ALU.S ← SA-SB = 5-5 = 0 (ALUSrc=0, ALUop=0001=SUB via raw_branch_op)
                      Zero=1, raw_result=Zero=1 (funct3[2]=0), branch_taken=XOR(1,0)=1
                      PCWrite=1, PCSrc=001 → PC ← PC_old+ImmExt = 12+8 = 20 (0x14)
                      RESET_CTR=1 (3-state cycle) → back to State 0
State 3 — absent
State 4 — absent
```

**Instruction 5: `addi x20, x20, 1`** (at address 0x14, `beq_ok`)
```
State 0 — Fetch     : IR ← 0x001A0A13, PC ← 24, PC_old ← 20
State 1 — Decode    : SA = X20 = 0, ImmExt = 1
State 2 — Execute   : ALUout ← 0+1 = 1 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X20 ← 1, RegWrite=1, RESET_CTR=1 → back to State 0
```

The same pattern repeats for BNE (+2), BLT (+4), BGE (+8), BLTU (+16), BGEU (+32), each driven by its own `funct3` selecting `raw_branch_op` and the comparison direction (signed/unsigned).

## 6. Test result

✅ **Passed** — `X20 = 63` (0b111111) after full execution, confirming that all 6 branch variants correctly trigger the jump in their respective cases:
```
BEQ (bit0) + BNE (bit1) + BLT (bit2) + BGE (bit3) + BLTU (bit4) + BGEU (bit5) = 63
```

This test specifically validates the signed/unsigned boundary case (`BLT`/`BGE` with `-1` and `1` producing an opposite result compared to `BLTU`/`BGEU` on the same raw bits), as well as the 3-state cycle specific to branches (`RESET_CTR` active as early as State 2, never reaching Memory or Writeback).
