# LW / SW — Load Word / Store Word

## 1. What these instructions do (official RISC-V spec)

```
SW rs2, imm(rs1) : Mem[rs1 + sext(imm[11:0])] ← rs2
LW rd,  imm(rs1) : rd ← sext(Mem[rs1 + sext(imm[11:0])])
```
Both compute the same effective address (`rs1 + imm`), but SW writes a full word to RAM while LW reads one and writes it into `rd`.

## 2. Binary decomposition

**SW — Type S format**
| Field | Bits | Role |
|---|---|---|
| opcode | `[6:0]` | `0100011` (0x23) |
| imm[4:0] | `[11:7]` | low part of the immediate |
| funct3 | `[14:12]` | `010` (word) |
| rs1 | `[19:15]` | address base |
| rs2 | `[24:20]` | value to store |
| imm[11:5] | `[31:25]` | high part of the immediate |

**LW — Type I format**
| Field | Bits | Role |
|---|---|---|
| opcode | `[6:0]` | `0000011` (0x03) |
| rd | `[11:7]` | destination register |
| funct3 | `[14:12]` | `010` (word) |
| rs1 | `[19:15]` | address base |
| imm[11:0] | `[31:20]` | signed offset |

**ImmSrc**: `001` for SW (Type S), `000` for LW (Type I).

## 3. Tested assembly code

```asm
addi x1, x0, 100    # x1 = 100  (base address)
addi x2, x0, 42      # x2 = 42   (value to store)
sw   x2, 0(x1)       # RAM[100] = 42
lw   x3, 0(x1)       # x3 = RAM[100] = 42
```

**Verified encoding:**
```
0x0000 : 06400093  (addi)
0x0004 : 02A00113  (addi)
0x0008 : 0020A023  (sw)
0x000C : 0000A183  (lw)
```

## 4. Instruction-specific control signals

| Signal | SW | LW |
|---|---|---|
| `ImmSrc` | `001` (Type S) | `000` (Type I) |
| `ALUSrc` | 1 | 1 |
| `ALUop` | `0000` (ADD, effective address) | `0000` (ADD) |
| `MARWrite` | `AND(isState2, isStore)` = 1 | `AND(isState2, isLoad)` = 1 |
| `MemWrite` | `AND(isState3, isStore)` = 1 | 0 |
| `MDRWrite` | 0 | `AND(isState3, isLoad)` = 1 |
| `WBSrc` | no effect (no RegWrite) | `001` (MDR) |
| `RegWrite` | 0 | `AND(isState4, isLoad)` = 1 |
| `RESET_CTR` | active as early as **State 3** (4-state cycle) | active at **State 4** (5-state cycle) |

## 5. Critical design point (bug fixed during this test)

`MAR` must be fed by the **raw combinational output of the ALU** (`ALU.S`), not by `ALUout.Q`. `MARWrite` and `ALUoutWrite` are both active at State 2: if `MAR.D` pointed to `ALUout.Q`, it would capture the value from the **previous** instruction (since `ALUout` only updates its output *after* the same clock edge). Both registers must share the same combinational source to capture the new address in parallel, on the same clock edge.

## 6. Full program — state by state

**Instruction 1: `addi x1, x0, 100`** — add immediate: `x1 ← x0 + 100` (base address)
```
State 0 — Fetch     : IR ← 0x06400093, PC ← 4, PC_old ← 0
State 1 — Decode    : SA = X0 = 0, ImmExt = 100 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+100 = 100 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X1 ← 100, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 2: `addi x2, x0, 42`** — add immediate: `x2 ← x0 + 42` (value to store)
```
State 0 — Fetch     : IR ← 0x02A00113, PC ← 8, PC_old ← 4
State 1 — Decode    : SA = X0 = 0, ImmExt = 42 (ImmSrc=000)
State 2 — Execute   : ALUout ← SA+ImmExt = 0+42 = 42 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X2 ← 42, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 3: `sw x2, 0(x1)`** — Store Word: `Mem[x1+0] ← x2`
```
State 0 — Fetch     : IR ← 0x0020A023, PC ← 12, PC_old ← 8
State 1 — Decode    : SA = X1 = 100, SB = X2 = 42, ImmExt = 0 (ImmSrc=001, Type S)
State 2 — Execute   : ALU.S ← SA+ImmExt = 100+0 = 100 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 100 (MARWrite=1, direct capture from ALU.S)
State 3 — Memory    : StoreMerger(SB=42, funct3=010=SW) → store_data=42
                      RAM[100] ← 42 (MemWrite=1), RESET_CTR=1 → back to State 0
State 4 — absent (4-state cycle, no Writeback for Store)
```

**Instruction 4: `lw x3, 0(x1)`** — Load Word: `x3 ← Mem[x1+0]`
```
State 0 — Fetch     : IR ← 0x0000A183, PC ← 16, PC_old ← 12
State 1 — Decode    : SA = X1 = 100, ImmExt = 0 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 100+0 = 100 (ALUSrc=1, ALUop=0000=ADD)
                      MAR ← 100 (MARWrite=1, direct capture from ALU.S)
State 3 — Memory    : MemAddrSrc=1 → RAM address = MAR>>2
                      LoadExtender(RAM.Q, byte_offset=00, funct3=010=LW) → load_data = 42
                      MDR ← 42 (MDRWrite=1)
State 4 — Writeback : X3 ← MDR = 42 (WBSrc=001, RegWrite=1), RESET_CTR=1 → back to State 0
```

24 clock ticks total (5+5+4+5, Store only having 4 states).

## 7. Test result

✅ **Passed** — `X1=100, X2=42, X3=42` after the 24 ticks.

This test confirms:
- Correct `MAR` timing (bug fixed, see §5)
- The `StoreMerger` path for an aligned word (SW)
- The `LoadExtender` path for an aligned word (LW)
- The 4-state cycle for Store (`RESET_CTR` as early as State 3, no Writeback)
- The `sel_ram` MMIO decoder (address 100 < 0x10000, correctly routed to RAM and not to MMIO)

**Not covered by this test** (tested separately): LB/LH/LBU/LHU/SB/SH (sub-word accesses within a word), and MMIO access (buttons/LED/counter) via LW/SW at addresses ≥ 0x10000.
