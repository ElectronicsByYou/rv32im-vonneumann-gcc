# Type I — Full grouped test (9 arithmetic operations)

## 1. What each instruction does (official RISC-V spec)

```
ADDI  rd, rs1, imm : rd ← rs1 + sext(imm[11:0])
ANDI  rd, rs1, imm : rd ← rs1 & sext(imm[11:0])
ORI   rd, rs1, imm : rd ← rs1 | sext(imm[11:0])
XORI  rd, rs1, imm : rd ← rs1 ^ sext(imm[11:0])
SLLI  rd, rs1, shamt : rd ← rs1 << shamt[4:0]                (logical left shift)
SRLI  rd, rs1, shamt : rd ← rs1 >> shamt[4:0]  (logical)      (fills with 0)
SRAI  rd, rs1, shamt : rd ← rs1 >> shamt[4:0]  (arithmetic)   (propagates sign bit)
SLTI  rd, rs1, imm : rd ← (rs1 < imm) ? 1 : 0                 (signed compare)
SLTIU rd, rs1, imm : rd ← (rs1 < imm) ? 1 : 0                 (unsigned compare)
```

## 2. Binary decomposition — Type I format (common to all 9)

| Field | Bits | Role |
|---|---|---|
| opcode | `[6:0]` | `0010011` (0x13), fixed for all |
| rd | `[11:7]` | destination register |
| funct3 | `[14:12]` | distinguishes the operation |
| rs1 | `[19:15]` | source register |
| imm[11:0] | `[31:20]` | signed immediate (or shamt[4:0]+funct7 for shifts) |

**SLLI/SRLI/SRAI special case**: the spec reuses `IR[31:25]` as a pseudo-`funct7` (`0000000` for SLLI/SRLI, `0100000` for SRAI), and only `IR[24:20]` encodes the actual shift amount (`shamt`).

## 3. Tested assembly code

```asm
addi x1, x0, 12       # x1 = 12

addi x2, x1, 8         # x2 = 12 + 8   = 20
andi x3, x1, 10        # x3 = 12 & 10  = 8
ori  x4, x1, 3         # x4 = 12 | 3   = 15
xori x5, x1, 10        # x5 = 12 ^ 10  = 6
slli x6, x1, 2         # x6 = 12 << 2  = 48
srli x7, x1, 2         # x7 = 12 >> 2 (logical)  = 3
srai x8, x1, 2         # x8 = 12 >> 2 (arithmetic) = 3
slti x9, x1, 20        # x9 = (12 < 20) signed     = 1
sltiu x10, x1, 20      # x10 = (12 < 20) unsigned  = 1

addi x11, x0, -1       # x11 = 0xFFFFFFFF (-1)
slti  x12, x11, 1      # x12 = (-1 < 1) signed         = 1
sltiu x13, x11, 1      # x13 = (0xFFFFFFFF < 1) unsigned = 0
```

## 4. ALUop ROM addresses exercised by this test

| Address | `{bit5,funct3}` | Instruction | ALUop value |
|---|---|---|---|
| 0 | 0000 | ADDI | 0000 (ADD) |
| 7 | 0111 | ANDI | 0010 (AND) |
| 6 | 0110 | ORI | 0011 (OR) |
| 4 | 0100 | XORI | 0100 (XOR) |
| 1 | 0001 | SLLI | 0101 (SLL) |
| 5 | 0101 | SRLI | 0110 (SRL) |
| D | 1101 | SRAI | 0111 (SRA) |
| 2 | 0010 | SLTI | 1000 (SLT) |
| 3 | 0011 | SLTIU | 1001 (SLTU) |

These addresses are identical to those used on the Type R side (same shared ROM) — this test therefore revalidates the ROM through a `isShiftImm`/`isTypeI` path distinct from `isTypeR`.

## 5. Full program — state by state

**Instruction 1: `addi x1, x0, 12`** — add immediate: `x1 ← x0 + 12`
```
State 0 — Fetch     : IR ← 0x00C00093, PC ← 4, PC_old ← 0
State 1 — Decode    : SA = X0 = 0, ImmExt = 12 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+12 = 12 (ALUSrc=1, ALUop=ROM{0,000}=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X1 ← 12, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 2: `addi x2, x1, 8`** — add immediate: `x2 ← x1 + 8`
```
State 0 — Fetch     : IR ← 0x00808113, PC ← 8, PC_old ← 4
State 1 — Decode    : SA = X1 = 12, ImmExt = 8 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 12+8 = 20 (ALUSrc=1, ALUop=ROM{0,000}=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X2 ← 20, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 3: `andi x3, x1, 10`** — bitwise AND with immediate: `x3 ← x1 & 10`
```
State 0 — Fetch     : IR ← 0x00A0F193, PC ← 12, PC_old ← 8
State 1 — Decode    : SA = X1 = 12, ImmExt = 10 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA&ImmExt = 12&10 = 8 (ALUSrc=1, ALUop=ROM{0,111}=0010=AND)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X3 ← 8, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 4: `ori x4, x1, 3`** — bitwise OR with immediate: `x4 ← x1 | 3`
```
State 0 — Fetch     : IR ← 0x0030E213, PC ← 16, PC_old ← 12
State 1 — Decode    : SA = X1 = 12, ImmExt = 3 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA|ImmExt = 12|3 = 15 (ALUSrc=1, ALUop=ROM{0,110}=0011=OR)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X4 ← 15, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 5: `xori x5, x1, 10`** — bitwise XOR with immediate: `x5 ← x1 ^ 10`
```
State 0 — Fetch     : IR ← 0x00A0C293, PC ← 20, PC_old ← 16
State 1 — Decode    : SA = X1 = 12, ImmExt = 10 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA^ImmExt = 12^10 = 6 (ALUSrc=1, ALUop=ROM{0,100}=0100=XOR)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X5 ← 6, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 6: `slli x6, x1, 2`** — logical left shift with immediate: `x6 ← x1 << 2`
```
State 0 — Fetch     : IR ← 0x00209313, PC ← 24, PC_old ← 20
State 1 — Decode    : SA = X1 = 12, ImmExt = 2 (ImmSrc=000, shamt on ImmExt[4:0])
State 2 — Execute   : ALUout ← SA<<(ImmExt&0x1F) = 12<<2 = 48 (ALUSrc=1, isShiftImm active,
                      ALUop=ROM{0,001}=0101=SLL)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X6 ← 48, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 7: `srli x7, x1, 2`** — logical right shift with immediate (fills with 0): `x7 ← x1 >> 2`
```
State 0 — Fetch     : IR ← 0x0020D393, PC ← 28, PC_old ← 24
State 1 — Decode    : SA = X1 = 12, ImmExt = 2 (ImmSrc=000, shamt on ImmExt[4:0])
State 2 — Execute   : ALUout ← SA>>(ImmExt&0x1F) logical = 12>>2 = 3 (ALUSrc=1, isShiftImm active,
                      bit5_gated=0 (IR[30]=0), ALUop=ROM{0,101}=0110=SRL)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X7 ← 3, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 8: `srai x8, x1, 2`** — arithmetic right shift with immediate (propagates sign): `x8 ← x1 >>> 2`
```
State 0 — Fetch     : IR ← 0x4020D413, PC ← 32, PC_old ← 28
State 1 — Decode    : SA = X1 = 12, ImmExt = 2 (ImmSrc=000, shamt on ImmExt[4:0])
State 2 — Execute   : ALUout ← SA>>(ImmExt&0x1F) arithmetic = 12>>2 = 3 (ALUSrc=1, isShiftImm active,
                      bit5_gated=1 (IR[30]=1), ALUop=ROM{1,101}=0111=SRA)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X8 ← 3, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 9: `slti x9, x1, 20`** — signed compare with immediate: `x9 ← (x1 < 20) ? 1 : 0`
```
State 0 — Fetch     : IR ← 0x0140A493, PC ← 36, PC_old ← 32
State 1 — Decode    : SA = X1 = 12, ImmExt = 20 (ImmSrc=000)
State 2 — Execute   : ALUout ← (SA<ImmExt, signed) = (12<20) = 1 (ALUSrc=1, ALUop=ROM{0,010}=1000=SLT)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X9 ← 1, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 10: `sltiu x10, x1, 20`** — unsigned compare with immediate: `x10 ← (x1 < 20) ? 1 : 0`
```
State 0 — Fetch     : IR ← 0x0140B513, PC ← 40, PC_old ← 36
State 1 — Decode    : SA = X1 = 12, ImmExt = 20 (ImmSrc=000)
State 2 — Execute   : ALUout ← (SA<ImmExt, unsigned) = (12<20) = 1 (ALUSrc=1, ALUop=ROM{0,011}=1001=SLTU)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X10 ← 1, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 11: `addi x11, x0, -1`** — add negative immediate: `x11 ← x0 + (-1)`
```
State 0 — Fetch     : IR ← 0xFFF00593, PC ← 44, PC_old ← 40
State 1 — Decode    : SA = X0 = 0, ImmExt = -1 = 0xFFFFFFFF (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+(-1) = -1 = 0xFFFFFFFF (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X11 ← 0xFFFFFFFF, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 12: `slti x12, x11, 1`** — signed compare: `x12 ← (x11 < 1) ? 1 : 0`, signed boundary case
```
State 0 — Fetch     : IR ← 0x0015A613, PC ← 48, PC_old ← 44
State 1 — Decode    : SA = X11 = 0xFFFFFFFF (-1 signed), ImmExt = 1 (ImmSrc=000)
State 2 — Execute   : ALUout ← (SA<ImmExt, signed) = (-1<1) = 1 (ALUSrc=1, ALUop=ROM{0,010}=1000=SLT)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X12 ← 1, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 13: `sltiu x13, x11, 1`** — unsigned compare: `x13 ← (x11 < 1) ? 1 : 0`, same boundary case in unsigned
```
State 0 — Fetch     : IR ← 0x0015B693, PC ← 52, PC_old ← 48
State 1 — Decode    : SA = X11 = 0xFFFFFFFF (4294967295 unsigned), ImmExt = 1 (ImmSrc=000)
State 2 — Execute   : ALUout ← (SA<ImmExt, unsigned) = (4294967295<1) = 0 (ALUSrc=1, ALUop=ROM{0,011}=1001=SLTU)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X13 ← 0, RegWrite=1, RESET_CTR=1 → back to State 0
```

65 clock ticks total (5 per instruction × 13).

## 6. Test result

✅ **Passed** — all results match, no additional bugs found:
```
X1=12, X2=20, X3=8, X4=15, X5=6, X6=48, X7=3, X8=3,
X9=1, X10=1, X11=0xFFFFFFFF(-1), X12=1, X13=0
```

This also confirms that the two fixes made during the Type R test (ROM address D for SRA/SRAI, zero-extend for SLT/SLTI and SLTU/SLTIU) hold correctly for the equivalent Type I instructions — the `isShiftImm`/`bit5_gated` mechanisms and the shared SLT/SLTU path are both exercised successfully here as well.

---

## Cumulative summary

✅ **19/19 Type R + Type I instructions validated** (10 Type R + 9 Type I).

Next step in the test plan: **LW** (Load), which will for the first time exercise the memory access state (state 3 actually active) and the `LoadExtender` path.
