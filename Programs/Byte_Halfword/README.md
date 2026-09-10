# LB / LH / LBU / LHU / SB / SH — Unaligned Sub-Word Memory Access

## 1. What these instructions do (official RISC-V spec)

```
SB rs2, imm(rs1) : Mem[rs1+imm][7:0]  ← rs2[7:0]           (stores 1 byte, always the low byte of rs2)
SH rs2, imm(rs1) : Mem[rs1+imm][15:0] ← rs2[15:0]           (stores 2 bytes, always the low 16 bits of rs2)

LB  rd, imm(rs1) : rd ← sext(Mem[rs1+imm][7:0])              (loads 1 byte, sign-extends)
LBU rd, imm(rs1) : rd ← zext(Mem[rs1+imm][7:0])              (loads 1 byte, zero-extends)
LH  rd, imm(rs1) : rd ← sext(Mem[rs1+imm][15:0])             (loads 2 bytes, sign-extends)
LHU rd, imm(rs1) : rd ← zext(Mem[rs1+imm][15:0])             (loads 2 bytes, zero-extends)
```

**Key spec point for SB/SH**: it is always the **low-order** byte/halfword of `rs2` that gets written, regardless of the destination address — the upper bits of `rs2` are ignored by the instruction.

## 2. Binary decomposition — Type I (Load) / Type S (Store) format

Identical to LW/SW, only `funct3` changes:
```
funct3 = 000 → LB   funct3 = 100 → LBU
funct3 = 001 → LH   funct3 = 101 → LHU
funct3 = 010 → LW/SW (already tested)
```

## 3. Tested assembly code

```asm
addi x1, x0, 200      # x1 = base address

addi x2, x0, -1        # x2 = 0xFFFFFFFF
sb   x2, 0(x1)         # RAM[200] byte = 0xFF

addi x3, x0, 127       # x3 = 0x7F
sb   x3, 1(x1)         # RAM[201] byte = 0x7F

lb   x4, 0(x1)         # x4 = sext(0xFF) = -1
lbu  x5, 0(x1)         # x5 = zext(0xFF) = 255
lb   x6, 1(x1)         # x6 = sext(0x7F) = 127
lbu  x7, 1(x1)         # x7 = zext(0x7F) = 127

addi x8, x0, -1        # x8 = 0xFFFFFFFF
sh   x8, 4(x1)         # RAM[204..205] = 0xFFFF

lh   x9, 4(x1)          # x9 = sext(0xFFFF) = -1
lhu  x10, 4(x1)         # x10 = zext(0xFFFF) = 65535
```

## 4. Full program — state by state

**Instruction 1: `addi x1, x0, 200`** — add immediate: `x1 ← x0 + 200` (base address)
```
State 0 — Fetch     : IR ← 0x0C800093, PC ← 4, PC_old ← 0
State 1 — Decode    : SA = X0 = 0, ImmExt = 200 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+200 = 200 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X1 ← 200, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 2: `addi x2, x0, -1`** — add immediate: `x2 ← x0 + (-1)`
```
State 0 — Fetch     : IR ← 0xFFF00113, PC ← 8, PC_old ← 4
State 1 — Decode    : SA = X0 = 0, ImmExt = -1 = 0xFFFFFFFF (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+(-1) = -1 = 0xFFFFFFFF (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X2 ← 0xFFFFFFFF, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 3: `sb x2, 0(x1)`** — Store Byte: `Mem[x1+0][7:0] ← x2[7:0]`
```
State 0 — Fetch     : IR ← 0x00208023, PC ← 12, PC_old ← 8
State 1 — Decode    : SA = X1 = 200, SB = X2 = 0xFFFFFFFF, ImmExt = 0 (ImmSrc=001, Type S)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+0 = 200 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 200 (MARWrite=1, direct capture from ALU.S)
State 3 — Memory    : byte_offset = MAR[1:0] = 00
                      StoreMerger(SB[7:0]=0xFF, byte_offset=00, funct3=000=SB) → store_data
                      RAM[200 word] ← merged word (byte 0 = 0xFF), MemWrite=1, RESET_CTR=1 → back to State 0
State 4 — absent (4-state cycle, no Writeback for Store)
```

**Instruction 4: `addi x3, x0, 127`** — add immediate: `x3 ← x0 + 127`
```
State 0 — Fetch     : IR ← 0x07F00193, PC ← 16, PC_old ← 12
State 1 — Decode    : SA = X0 = 0, ImmExt = 127 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+127 = 127 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X3 ← 127, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 5: `sb x3, 1(x1)`** — Store Byte: `Mem[x1+1][7:0] ← x3[7:0]`
```
State 0 — Fetch     : IR ← 0x003080A3, PC ← 20, PC_old ← 16
State 1 — Decode    : SA = X1 = 200, SB = X3 = 127, ImmExt = 1 (ImmSrc=001, Type S)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+1 = 201 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 201 (MARWrite=1, direct capture from ALU.S)
State 3 — Memory    : byte_offset = MAR[1:0] = 01
                      StoreMerger(SB[7:0]=0x7F, byte_offset=01, funct3=000=SB) → store_data
                      RAM[200 word] ← merged word (byte 1 = 0x7F, byte 0 unchanged = 0xFF), MemWrite=1,
                      RESET_CTR=1 → back to State 0
State 4 — absent (4-state cycle)
```

**Instruction 6: `lb x4, 0(x1)`** — Load Byte (signed): `x4 ← sext(Mem[x1+0][7:0])`
```
State 0 — Fetch     : IR ← 0x00008203, PC ← 24, PC_old ← 20
State 1 — Decode    : SA = X1 = 200, ImmExt = 0 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+0 = 200 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 200 (MARWrite=1)
State 3 — Memory    : byte_offset = MAR[1:0] = 00
                      LoadExtender(RAM.Q, byte_offset=00, funct3=000=LB) → sign-extend byte 0xFF → load_data = 0xFFFFFFFF
                      MDR ← 0xFFFFFFFF (MDRWrite=1)
State 4 — Writeback : X4 ← MDR = 0xFFFFFFFF (-1) (WBSrc=001, RegWrite=1), RESET_CTR=1 → back to State 0
```

**Instruction 7: `lbu x5, 0(x1)`** — Load Byte Unsigned: `x5 ← zext(Mem[x1+0][7:0])`
```
State 0 — Fetch     : IR ← 0x0000C283, PC ← 28, PC_old ← 24
State 1 — Decode    : SA = X1 = 200, ImmExt = 0 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+0 = 200 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 200 (MARWrite=1)
State 3 — Memory    : byte_offset = MAR[1:0] = 00
                      LoadExtender(RAM.Q, byte_offset=00, funct3=100=LBU) → zero-extend byte 0xFF → load_data = 0x000000FF
                      MDR ← 0x000000FF (MDRWrite=1)
State 4 — Writeback : X5 ← MDR = 255 (WBSrc=001, RegWrite=1), RESET_CTR=1 → back to State 0
```

**Instruction 8: `lb x6, 1(x1)`** — Load Byte (signed): `x6 ← sext(Mem[x1+1][7:0])`
```
State 0 — Fetch     : IR ← 0x00108303, PC ← 32, PC_old ← 28
State 1 — Decode    : SA = X1 = 200, ImmExt = 1 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+1 = 201 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 201 (MARWrite=1)
State 3 — Memory    : byte_offset = MAR[1:0] = 01
                      LoadExtender(RAM.Q, byte_offset=01, funct3=000=LB) → sign-extend byte 0x7F → load_data = 0x0000007F
                      MDR ← 0x0000007F (MDRWrite=1)
State 4 — Writeback : X6 ← MDR = 127 (WBSrc=001, RegWrite=1), RESET_CTR=1 → back to State 0
```

**Instruction 9: `lbu x7, 1(x1)`** — Load Byte Unsigned: `x7 ← zext(Mem[x1+1][7:0])`
```
State 0 — Fetch     : IR ← 0x0010C383, PC ← 36, PC_old ← 32
State 1 — Decode    : SA = X1 = 200, ImmExt = 1 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+1 = 201 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 201 (MARWrite=1)
State 3 — Memory    : byte_offset = MAR[1:0] = 01
                      LoadExtender(RAM.Q, byte_offset=01, funct3=100=LBU) → zero-extend byte 0x7F → load_data = 0x0000007F
                      MDR ← 0x0000007F (MDRWrite=1)
State 4 — Writeback : X7 ← MDR = 127 (WBSrc=001, RegWrite=1), RESET_CTR=1 → back to State 0
```

**Instruction 10: `addi x8, x0, -1`** — add immediate: `x8 ← x0 + (-1)`
```
State 0 — Fetch     : IR ← 0xFFF00413, PC ← 40, PC_old ← 36
State 1 — Decode    : SA = X0 = 0, ImmExt = -1 = 0xFFFFFFFF (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+(-1) = -1 = 0xFFFFFFFF (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X8 ← 0xFFFFFFFF, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 11: `sh x8, 4(x1)`** — Store Halfword: `Mem[x1+4][15:0] ← x8[15:0]`
```
State 0 — Fetch     : IR ← 0x00809223, PC ← 44, PC_old ← 40
State 1 — Decode    : SA = X1 = 200, SB = X8 = 0xFFFFFFFF, ImmExt = 4 (ImmSrc=001, Type S)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+4 = 204 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 204 (MARWrite=1)
State 3 — Memory    : byte_offset = MAR[1:0] = 00
                      StoreMerger(SB[15:0]=0xFFFF, byte_offset=00, funct3=001=SH) → store_data
                      RAM[204 word] ← merged word (halfword 0 = 0xFFFF), MemWrite=1, RESET_CTR=1 → back to State 0
State 4 — absent (4-state cycle)
```

**Instruction 12: `lh x9, 4(x1)`** — Load Halfword (signed): `x9 ← sext(Mem[x1+4][15:0])`
```
State 0 — Fetch     : IR ← 0x00409483, PC ← 48, PC_old ← 44
State 1 — Decode    : SA = X1 = 200, ImmExt = 4 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+4 = 204 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 204 (MARWrite=1)
State 3 — Memory    : byte_offset = MAR[1:0] = 00
                      LoadExtender(RAM.Q, byte_offset=00, funct3=001=LH) → sign-extend halfword 0xFFFF → load_data = 0xFFFFFFFF
                      MDR ← 0xFFFFFFFF (MDRWrite=1)
State 4 — Writeback : X9 ← MDR = 0xFFFFFFFF (-1) (WBSrc=001, RegWrite=1), RESET_CTR=1 → back to State 0
```

**Instruction 13: `lhu x10, 4(x1)`** — Load Halfword Unsigned: `x10 ← zext(Mem[x1+4][15:0])`
```
State 0 — Fetch     : IR ← 0x0040D503, PC ← 52, PC_old ← 48
State 1 — Decode    : SA = X1 = 200, ImmExt = 4 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 200+4 = 204 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 204 (MARWrite=1)
State 3 — Memory    : byte_offset = MAR[1:0] = 00
                      LoadExtender(RAM.Q, byte_offset=00, funct3=101=LHU) → zero-extend halfword 0xFFFF → load_data = 0x0000FFFF
                      MDR ← 0x0000FFFF (MDRWrite=1)
State 4 — Writeback : X10 ← MDR = 65535 (WBSrc=001, RegWrite=1), RESET_CTR=1 → back to State 0
```

61 clock ticks total (5+5+4+5+4+5+5+5+5+5+4+5+5).

Final expected result: **X1=200, X2=-1, X3=127, X4=-1, X5=255, X6=127, X7=127, X8=-1, X9=-1, X10=65535**.

## 5. Test result

✅ **Passed** — all results match expectations:
```
X1=200, X2=-1, X3=127, X4=-1, X5=255, X6=127, X7=127, X8=-1, X9=-1, X10=65535
```

This test confirms:
- `byte_offset` (`MAR[1:0]`) is correctly used on both the read path (`LoadExtender`) and the write path (`StoreMerger`), for two different bytes within the same aligned word (addresses 200 and 201).
- Sign-extend and zero-extend work correctly for LB/LBU (`0xFF` → `-1` vs `255`) and LH/LHU (`0xFFFF` → `-1` vs `65535`).
- The `StoreMerger` fusion mechanism (read-modify-write) correctly preserves untouched bytes (byte0 stays `0xFF` intact after the second `SB`).

## 6. Follow-up test — all positions in a single pass

To explicitly cover all 4 byte positions (0,1,2,3) and both halfword positions (0,2) with distinct values (catches any position permutation):

```asm
addi x1, x0, 300      # base address (word-aligned)

addi x2, x0, 0x11 ; sb x2, 0(x1)   # position 0
addi x3, x0, 0x22 ; sb x3, 1(x1)   # position 1
addi x4, x0, 0x33 ; sb x4, 2(x1)   # position 2
addi x5, x0, 0x44 ; sb x5, 3(x1)   # position 3

lbu x6, 0(x1)   # 17          lbu x7, 1(x1)   # 34
lbu x8, 2(x1)   # 51          lbu x9, 3(x1)   # 68
lw  x10, 0(x1)  # diagnostic: 1144201745 (0x44332211)

addi x11, x0, 500 ; sh x11, 4(x1)   # halfword position 0
addi x12, x0, 999 ; sh x12, 6(x1)   # halfword position 2
lhu x13, 4(x1)  # 500          lhu x14, 6(x1)  # 999
lw  x15, 4(x1)  # diagnostic: 65470964 (0x03E701F4)
```

✅ **Passed** — all results match, no further bugs:
```
X6=17, X7=34, X8=51, X9=68, X10=1144201745,
X11=500, X12=999, X13=500, X14=999, X15=65470964
```
The two full-word diagnostics (`X10`, `X15`) confirm the absence of any byte or halfword position permutation — the `byte_offset`/Decoder mechanism is therefore exhaustively validated across all 4 possible positions.
