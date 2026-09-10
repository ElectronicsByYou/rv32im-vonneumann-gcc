# JAL / JALR — Jump and Link / Jump and Link Register

## 1. What these instructions do (official RISC-V spec)

```
JAL  rd, imm      : rd ← PC+4 ; PC ← PC + sext(imm)
JALR rd, rs1, imm : rd ← PC+4 ; PC ← (rs1 + sext(imm)) & ~1
```
Both are **unconditional** jumps that save the address of the following instruction into `rd` — unlike Branch, which never saves anything. JAL jumps to a fixed target encoded in the instruction (relative to `PC`), JALR jumps to a dynamically computed address (`rs1+imm`), with bit 0 of the result forced to 0.

## 2. Binary decomposition

**JAL — Type J format**
| Field | Bits | Role |
|---|---|---|
| opcode | `[6:0]` | `1101111` (0x6F) |
| rd | `[11:7]` | receives `PC+4` |
| imm[19:12], imm[11], imm[10:1], imm[20] | `[31:12]` (reordered) | signed offset relative to PC |

**JALR — Type I format**
| Field | Bits | Role |
|---|---|---|
| opcode | `[6:0]` | `1100111` (0x67) |
| rd | `[11:7]` | receives `PC+4` |
| funct3 | `[14:12]` | `000` |
| rs1 | `[19:15]` | target address base |
| imm[11:0] | `[31:20]` | signed offset |

## 3. Tested assembly code

```asm
addi x10, x0, 5         # a0 = 5

jal  x6, double_it        # call: x6 = return address (PC+4), rd != ra on purpose
addi x7, x0, 999           # should be skipped by the +4-offset return
addi x28, x0, 111          # should be executed (proof the +4 jump succeeded)

jal  x0, end                # skip over the function body
double_it:
    slli x10, x10, 1          # a0 = a0*2 = 10
    jalr x0, x6, 4              # return with a +4 offset (not +0), skips the next instruction

end:
    jal x0, end
```

**Verified encoding:**
```
0x0000 : 00500513  (addi)
0x0004 : 0100036F  (jal)
0x0008 : 3E700393  (addi)
0x000C : 06F00E13  (addi)
0x0010 : 00C0006F  (jal)
0x0014 : 00151513  (slli)
0x0018 : 00430067  (jalr)
0x001C : 0000006F  (jal)
```

## 4. Instruction-specific control signals

| Signal | JAL | JALR |
|---|---|---|
| `ImmSrc` | `100` (Type J) | `000` (Type I) |
| `ALUSrc` | no effect (doesn't use the ALU) | 1 |
| `PCWrite` | `AND(isState4, isJAL)` = 1 | `AND(isState4, isJALR)` = 1 |
| `PCSrc` | `001` (PC_old+ImmExt), gated by `isState4` | `010` (ALUout, bit 0 clamped to 0) |
| `WBSrc` | `010` (PC_old+4) | `010` (PC_old+4) |
| `RegWrite` | 1 | 1 |
| `RESET_CTR` | active at `isState4` (5-state cycle, traverses Memory empty) | same |

## 5. Full program — state by state

**Instruction 1: `addi x10, x0, 5`** — initializes a0
```
State 0 — Fetch     : IR ← 0x00500513, PC ← 4, PC_old ← 0
State 1 — Decode    : SA = X0 = 0, ImmExt = 5
State 2 — Execute   : ALUout ← 0+5 = 5 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X10 ← 5, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 2: `jal x6, double_it`** — calls the function, saves the return address into x6
```
State 0 — Fetch     : IR ← 0x0100036F, PC ← 8, PC_old ← 4
State 1 — Decode    : ImmExt = 16 (0x10, offset to double_it), ImmSrc=100 (Type J)
State 2 — Execute   : nothing useful on the ALU (dedicated path)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X6 ← PC_old+4 = 4+4 = 8 (WBSrc=010, RegWrite=1)
                      PC ← PC_old+ImmExt = 4+16 = 20 (0x14) (PCSrc=001, PCWrite=1)
                      RESET_CTR=1 → back to State 0
```

**Instruction 3 (at 0x14): `slli x10, x10, 1`** — doubles a0
```
State 0 — Fetch     : IR ← 0x00151513, PC ← 24, PC_old ← 20
State 1 — Decode    : SA = X10 = 5, ImmExt = 1 (shamt)
State 2 — Execute   : ALUout ← SA<<1 = 5<<1 = 10 (ALUSrc=1, isShiftImm, ALUop=0101=SLL)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X10 ← 10, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 4 (at 0x18): `jalr x0, x6, 4`** — returns with a +4 offset, skips the next instruction
```
State 0 — Fetch     : IR ← 0x00430067, PC ← 28, PC_old ← 24
State 1 — Decode    : SA = X6 = 8, ImmExt = 4 (ImmSrc=000, Type I)
State 2 — Execute   : ALU.S ← SA+ImmExt = 8+4 = 12 (0x0C), bit 0 already 0 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X0 ← PC_old+4 (discarded, x0 hardwired to 0)
                      PC ← ALUout = 12 (0x0C) (PCSrc=010, PCWrite=1)
                      RESET_CTR=1 → back to State 0
```

**Instruction 5 (at 0x0C, reached via the jump): `addi x28, x0, 111`** — executed thanks to the +4 jump
```
State 0 — Fetch     : IR ← 0x06F00E13, PC ← 16 (0x10), PC_old ← 12 (0x0C)
State 1 — Decode    : SA = X0 = 0, ImmExt = 111
State 2 — Execute   : ALUout ← 0+111 = 111 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X28 ← 111, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Note**: the instruction at `0x08` (`addi x7, x0, 999`) is **never executed** — the CPU jumps directly from `0x18` (JALR) to `0x0C`, skipping `0x08` entirely. `x7` therefore stays at its initial value (0).

## 6. Test result

✅ **Passed**:
```
X10 = 10   (function correctly executed: 5×2)
X6  = 8    (JAL correctly wrote PC+4 into an arbitrary rd, not just ra)
X7  = 0    (instruction correctly skipped by the +4 return)
X28 = 111  (nonzero JALR jump lands exactly at the right address)
```

This test specifically validates two points beyond the trivial `jalr x0, ra, 0` (simple `ret`) case:
- **JAL with an arbitrary destination register** (`x6`), proving that writing `PC+4` is not hardwired to `ra`.
- **JALR with a nonzero offset** (`+4`), validating the full `rs1+imm` computation rather than just the zero-offset special case.
