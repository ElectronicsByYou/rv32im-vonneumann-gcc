# LUI / AUIPC — Load Upper Immediate / Add Upper Immediate to PC

## 1. What these instructions do (official RISC-V spec)

```
LUI   rd, imm : rd ← imm[31:12] << 12
AUIPC rd, imm : rd ← PC + (imm[31:12] << 12)
```
Both place a 20-bit immediate into the upper bits of a register (the low 12 bits stay zero). LUI starts from zero, AUIPC starts from the current address (`PC`, captured via `PC_old`).

**Typical usage**: combined with `ADDI`, LUI allows loading any 32-bit constant (ADDI alone only covers a signed 12-bit range). AUIPC is used for computing code-relative addresses (position-independent code, or extending reach beyond JAL's ±1MB range when combined with JALR).

## 2. Binary decomposition — Type U format (common to both)

| Field | Bits | Role |
|---|---|---|
| opcode | `[6:0]` | `0110111` (LUI, 0x37) or `0010111` (AUIPC, 0x17) |
| rd | `[11:7]` | destination register |
| imm[31:12] | `[31:12]` | 20 bits copied directly (no sign-extend, just zero-padded below) |

**ImmSrc = `011`** (Type U) for both.

## 3. Tested assembly code

```asm
lui  x10, 0x12345         # x10 = 0x12345000
addi x10, x10, 0x678       # x10 = 0x12345678 (classic LUI+ADDI pattern)

lui  x11, 0xFFFFF           # x11 = 0xFFFFF000 (verifies the low bits are zero)

auipc x12, 0                # x12 = PC_old of this instruction (current address)
```

**Verified encoding:**
```
0x0000 : 12345537  (lui)
0x0004 : 67850513  (addi)
0x0008 : FFFFF5B7  (lui)
0x000C : 00000617  (auipc)
```

## 4. Instruction-specific control signals

| Signal | LUI | AUIPC |
|---|---|---|
| `ImmSrc` | `011` (Type U) | `011` (Type U) |
| `isAUIPC` (ALU A-input mux) | 0 (uses SA, no effect here) | 1 → selects `PC_old` instead of `SA` |
| `ALUSrc` | no effect (bypasses the ALU) | 1 (selects `ImmExt`) |
| `ALUop` | no effect | `0000` (ADD) |
| `WBSrc` | `011` (raw ImmExt, bypassing the ALU) | `000` (ALUout, result of `PC_old+ImmExt`) |
| `RegWrite` | 1 | 1 |
| `RESET_CTR` | active at `isState4` (5 states, Memory traversed empty) | same |

## 5. Full program — state by state

**Instruction 1: `lui x10, 0x12345`** — loads the upper bits of a constant
```
State 0 — Fetch     : IR ← 0x12345537, PC ← 4, PC_old ← 0
State 1 — Decode    : ImmExt = 0x12345000 (ImmSrc=011, Type U: IR[31:12] + 12 zeros)
State 2 — Execute   : nothing useful on the ALU (bypass path)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X10 ← ImmExt = 0x12345000 (WBSrc=011, RegWrite=1), RESET_CTR=1 → back to State 0
```

**Instruction 2: `addi x10, x10, 0x678`** — fills in the low bits
```
State 0 — Fetch     : IR ← 0x67850513, PC ← 8, PC_old ← 4
State 1 — Decode    : SA = X10 = 0x12345000, ImmExt = 0x678 (ImmSrc=000, Type I)
State 2 — Execute   : ALUout ← SA+ImmExt = 0x12345000+0x678 = 0x12345678 (ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X10 ← 0x12345678, RegWrite=1, RESET_CTR=1 → back to State 0
```

**Instruction 3: `lui x11, 0xFFFFF`** — verifies the low bits stay zero
```
State 0 — Fetch     : IR ← 0xFFFFF5B7, PC ← 12, PC_old ← 8
State 1 — Decode    : ImmExt = 0xFFFFF000 (ImmSrc=011, Type U)
State 2 — Execute   : nothing useful on the ALU
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X11 ← ImmExt = 0xFFFFF000 (WBSrc=011, RegWrite=1), RESET_CTR=1 → back to State 0
```

**Instruction 4: `auipc x12, 0`** — computes the current address
```
State 0 — Fetch     : IR ← 0x00000617, PC ← 16, PC_old ← 12
State 1 — Decode    : ImmExt = 0 (ImmSrc=011, Type U)
State 2 — Execute   : ALUout ← PC_old+ImmExt = 12+0 = 12 (0x0C) (isAUIPC=1 → mux A=PC_old, ALUSrc=1, ALUop=0000=ADD)
State 3 — Memory    : nothing (traversed empty)
State 4 — Writeback : X12 ← ALUout = 12 (WBSrc=000, RegWrite=1), RESET_CTR=1 → back to State 0
```

20 clock ticks total (5 per instruction × 4).

## 6. Test result

✅ **Passed**:
```
X10 = 0x12345678  (LUI+ADDI pattern, loads an arbitrary 32-bit constant)
X11 = 0xFFFFF000  (the low 12 bits stay correctly at zero, no residual bits)
X12 = 0x0000000C  (AUIPC correctly computes PC_old+ImmExt via the ALU's A-input mux)
```

This test validates a point critical to GCC compatibility: the startup code (`crt0.s`) systematically uses the `lui+addi` pattern to initialize the stack pointer (`sp`) and global pointer (`gp`) — without a working LUI, no GCC-compiled program can start at all, even before reaching `main()`.
