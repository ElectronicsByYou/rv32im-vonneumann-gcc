# RV32IM RISC-V CPU — Von Neumann Architecture (Logisim Evolution)

A multicycle RV32IM RISC-V CPU built from scratch in Logisim Evolution, using a **unified Von Neumann memory** (as opposed to an earlier Harvard-architecture attempt), designed from the ground up to run real **GCC-compiled C programs**.

Every instruction in this CPU was implemented and validated **one at a time**, state by state, against the official RISC-V specification — not copied from a reference design. This README traces the full build log: the datapath, the control unit, every test performed, every bug found and fixed, and the current status toward full GCC compatibility.

---

## 1. Architecture overview

- **ISA**: RV32IM (base integer + multiplication extension, partial)
- **Memory model**: Von Neumann — a single unified 64KB RAM holds both code and data
- **Datapath**: multicycle, 5-state finite state machine (Fetch → Decode → Execute → Memory → Writeback)
- **Address map**:
  ```
  0x00000000 – 0x0000FFFF → unified RAM (64KB): .text, .data, stack
  0x00010000 and above    → MMIO (buttons, LED matrix, counter)
  ```
- **Control unit**: hand-built combinational logic (no microcode ROM for the main control signals), derived directly from RISC-V opcodes, `funct3`, and `funct7`

## 2. FSM states

```
State 0 — Fetch     : IR ← RAM[PC], PC ← PC+4, PC_old ← PC (address of current instruction)
State 1 — Decode     : register file read, immediate extraction
State 2 — Execute     : ALU computation
State 3 — Memory      : RAM access (Load/Store only)
State 4 — Writeback    : register file write
```
Cycle length varies by instruction family:
- **Branch**: 3 states (0→1→2→0), no Memory or Writeback
- **Store**: 4 states (0→1→2→3→0), no Writeback
- **Everything else**: 5 states (0→1→2→3→4→0), with state 3 traversed empty when unused

## 3. Datapath components

| Block | Role |
|---|---|
| **PC / PC_old** | Program counter and captured address of the current instruction (needed for Branch/JAL/AUIPC PC-relative calculations) |
| **IR** | Instruction register, loaded at Fetch |
| **Register File** | 32×32-bit registers, x0 hardwired to zero, combinational read / synchronous write |
| **Sign Extender** | Produces `ImmExt` for all 5 immediate formats (I, S, B, U, J), each verified bit-for-bit against the spec |
| **ALU** | 11 operations (ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU, MUL), fully combinational |
| **MAR / MDR** | Memory address/data registers, fed directly from the ALU's raw combinational output (not the registered `ALUout`) to avoid a one-cycle timing bug (see §6) |
| **LoadExtender** | Sub-circuit: extracts and sign/zero-extends a byte or halfword from a 32-bit RAM word, based on `funct3` and the byte offset |
| **StoreMerger** | Sub-circuit: performs a read-modify-write to insert a byte or halfword into a 32-bit RAM word without disturbing the untouched bytes |
| **Control Unit** | Produces all 15 control signals from `opcode`, `funct3`, `funct7`, `Etat` (state), `Zero`, and `ALUResult` |

## 4. Instruction set — coverage status

### RV32I base — fully implemented and tested

| Category | Instructions | Status |
|---|---|---|
| R-type arithmetic | ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU | ✅ Tested |
| I-type arithmetic | ADDI, ANDI, ORI, XORI, SLLI, SRLI, SRAI, SLTI, SLTIU | ✅ Tested |
| Load | LB, LH, LW, LBU, LHU | ✅ Tested (all 4 byte positions + both halfword positions) |
| Store | SB, SH, SW | ✅ Tested (all 4 byte positions + both halfword positions) |
| Branch | BEQ, BNE, BLT, BGE, BLTU, BGEU | ✅ Tested |
| Jump | JAL, JALR | ✅ Tested (arbitrary `rd`, nonzero JALR offset) |
| Upper immediate | LUI, AUIPC | ✅ Tested |

### RV32M extension — partial

| Instruction | Status |
|---|---|
| MUL | ✅ Implemented, formal test pending |
| MULH, MULHSU, MULHU | ❌ Not implemented |
| DIV, DIVU, REM, REMU | ❌ Not implemented (see §7 for the GCC workaround) |

### Zicsr / privileged (interrupts) — deliberately deferred

| Instruction | Status |
|---|---|
| CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI | ❌ Not implemented |
| MRET | ❌ Not implemented |
| ECALL, EBREAK, FENCE | ❌ Not implemented |

**37 out of 47 possible RV32IM instructions are implemented and individually validated.**

## 5. Testing methodology

Every instruction (or logical group of instructions) has a dedicated markdown document in `/docs`, following a consistent format:
1. Official RISC-V semantics
2. Binary field decomposition
3. Assembled test program (built with a custom two-pass C assembler, see §8)
4. Control signal equations specific to that instruction
5. Full state-by-state trace (every clock tick, every register write)
6. Pass/fail result

For instructions with several sub-variants sharing a mechanism (e.g. the 6 branches, or the 10 R-type ALU ops), a **bitmask diagnostic technique** was used: each variant increments a result register by a distinct power of two, so a single final value reveals — bit by bit — exactly which variants pass or fail, without needing to re-run isolated tests one by one.

Beyond formal instruction tests, two small real programs were used as integration tests: an **iterative Fibonacci sequence** and **powers of two**, both pushed deliberately past 32-bit signed overflow to also exercise a separate 32-bit decimal display circuit (successive division by 10 via chained `Divider` components).

## 6. Bugs found and fixed during testing

This project deliberately tests every instruction against the spec rather than trusting the design — several real bugs were caught this way:

1. **MAR/ALUout timing race**: `MAR` was initially fed from `ALUout.Q` (the registered output) instead of the ALU's raw combinational output. Since `MARWrite` and `ALUoutWrite` are both active at the same state, `MAR` was capturing a one-cycle-stale value. Fixed by feeding both registers from the same combinational source.

2. **SLT/SLTU sign-extension bug**: the 1-bit comparator result was sign-extended instead of zero-extended, so a true comparison produced `0xFFFFFFFF` (-1) instead of `1`, violating the spec's explicit wording ("writing 1 to rd"). Fixed by switching the extension type to zero-extend.

3. **StoreMerger byte-fusion bug**: each of the 4 per-position merge muxes was wired to a *different* byte of the source register (`SB[15:8]`, `SB[23:16]`, etc.) instead of always using `SB[7:0]` — violating the spec, which states that SB/SH always write the *low-order* byte/halfword of the source register regardless of destination position.

4. **PCSrc missing state-gating**: `PCSrc`'s branch/jump terms were not gated by `isState2`/`isState4` the way `PCWrite` was. This let stale `IR` data (from the *previous* instruction, still present at the very start of the next Fetch) spuriously redirect the PC, causing certain instructions right after a branch to double-execute. Fixed by gating `PCSrc` with the same per-state conditions as `PCWrite`.

5. **ALUResult timing race**: same root cause as bug #1 — `ALUResult` (used by `branch_taken` for BLT/BGE/BLTU/BGEU) was read from `ALUout.Q[0]` instead of the ALU's raw output, causing branches that combine "ALUResult as source" with "inverted result" (BGE, BGEU) to use a stale comparison from the previous instruction.

Each of these is a reminder that **a wrong result is not always where you'd expect it** — isolating with independent diagnostics (raw hex word reads, bitmask counters) was consistently more reliable than guessing from symptoms alone.

## 7. Path to GCC compatibility

A real `riscv64-unknown-elf-gcc` toolchain (multilib `rv32im/ilp32`) was installed and verified to target this exact ISA/ABI combination.

**Confirmed to work without any extra hardware**:
- Integer arithmetic, including `/` and `%`, when compiled with **`-mno-div`** (forces GCC to emit a software division routine instead of the unimplemented `DIV`/`DIVU` instructions)
- `float`/`double` addition, subtraction, and multiplication, via GCC's software floating-point routines in `libgcc` (confirmed by disassembly: these routines only use RV32IM instructions already implemented)

**Known remaining risk**:
- `libgcc`'s **floating-point division** routine (`__divsf3`) and its **64-bit integer division** routines internally use the hardware `divu` instruction — confirmed present by disassembling the prebuilt `libgcc.a`. Since this library is precompiled assuming `-mdiv` is available (the default for the `rv32im` multilib), `-mno-div` on your own code does **not** protect against this. Avoid floating-point division and 64-bit (`long long`) division/modulo until a hardware divider is implemented.

**Confirmed blocking issue, not yet fixed**:
- The provided `crt0.s` startup code contains `csrci mstatus, 8` as its very first instruction. Since no CSR opcode is recognized by the control unit, this would hang the CPU indefinitely before even reaching `main()`. **This line must be removed** from `crt0.s` before any GCC-compiled program can run (safe to remove: with no CSR file, no interrupt can fire anyway).

**Not yet built**: `bin2logisim.py` (binary → `v2.0 raw` hex converter) and a full end-to-end compile-and-run test have not been executed yet.

## 8. Custom assembler

A separate two-pass C assembler (`lexer.c` / `symbols.c` / `encoder.c` / `output.c`) was used throughout testing to hand-assemble `.s` files into Logisim's `v2.0 raw` hex format, independently of GCC. It supports:
- All implemented RV32IM instructions
- ABI register names (`a0`, `t0`, `ra`, etc.)
- Labels and relative branch/jump resolution
- The `imm(rs)` addressing syntax for Load/Store/JALR

This tool is intentionally kept separate from the GCC pipeline — it exists for quick, direct hand-written test programs, while GCC's own assembler/linker handle real C compilation.

## 9. Repository structure

```
/docs/              — one markdown file per instruction (or instruction group), English + French versions
/asm/               — custom two-pass RISC-V assembler (C source)
/tests/             — .s source files and assembled .txt (v2.0 raw) test programs
/logisim/           — the .circ Logisim Evolution project file(s)
```

## 10. Roadmap

- [ ] Formal MUL test (currently implemented but not yet individually validated)
- [ ] MULH / MULHSU / MULHU (trivial addition — same `Multiplier` component, route the high 32 bits instead of the low 32)
- [ ] Hardware multicycle divider for DIV/DIVU/REM/REMU (non-trivial — requires FSM stalling logic)
- [ ] Remove the CSR line from `crt0.s`, write `bin2logisim.py`, run a full GCC → Logisim pipeline test
- [ ] CSR file + Zicsr instructions + MRET (interrupt support)
- [ ] Button/timer interrupt sources (`irq_ext`, `irq_timer`) wired into the CSR file
- [ ] Optional TTY output MMIO peripheral for character/string display
- [ ] Optional: recompile `libgcc` with `-mno-div` to fully eliminate the hardware-division dependency risk

---

*Built and debugged interactively, instruction by instruction, with strict adherence to the official RISC-V ISA specification at every step.*
