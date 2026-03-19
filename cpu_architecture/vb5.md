# PVDasm Visual Basic 5 P-Code Opcode Coverage

This document lists all VB5 P-Code opcodes supported by PVDasm. Every entry below has been verified against the source code in `VBPCode.cpp` and the auto-generated opcode table `VBOpcodeTable.h` (sourced from `vb_opcodes.json`).

P-Code is the bytecode produced when a VB5 project is compiled in "P-Code" mode (as opposed to Native Code). The VB5 runtime (`MSVBVM50.DLL`) interprets it at run-time.

> **Note:** VB5 and VB6 share an identical P-Code instruction set. The only difference is the runtime DLL (`MSVBVM50.DLL` vs `MSVBVM60.DLL`). See [`vb6.md`](vb6.md) for the complete opcode reference — all entries there apply to VB5 as well.

---

## Encoding

- **Base opcodes**: single byte `0x00`-`0xFA` (251 defined)
- **Extended opcodes**: two bytes — prefix `0xFB`-`0xFF` followed by a second byte `0x00`-`0xFF`
  - `0xFB xx` — Lead0 table (256 defined)
  - `0xFC xx` — Lead1 table (256 defined)
  - `0xFD xx` — Lead2 table (256 defined)
  - `0xFE xx` — Lead3 table (256 defined)
  - `0xFF xx` — Lead4 table (71 defined)
- **Total: 1346 opcodes** (545 unique mnemonics)
- Instruction length: 1-9 bytes (opcode header + inline arguments)

---

## Type Suffixes

Many opcodes exist in multiple typed variants. The suffix indicates the data type:

| Suffix | VB Type | Size |
|--------|---------|------|
| `I2` | Integer (16-bit) | 2 bytes |
| `I4` | Long (32-bit) | 4 bytes |
| `UI1` | Byte (unsigned 8-bit) | 1 byte |
| `R4` | Single (32-bit float) | 4 bytes |
| `R8` | Double (64-bit float) | 8 bytes |
| `FPR4` | Single on FP stack | 4 bytes |
| `FPR8` | Double on FP stack | 8 bytes |
| `Cy` | Currency (64-bit fixed) | 8 bytes |
| `Str` | String (BSTR) | pointer |
| `Var` | Variant (16-byte) | 16 bytes |
| `Ad` | Address/pointer | 4 bytes |
| `Bool` | Boolean | 2 bytes |
| `Date` | Date (Double) | 8 bytes |

---

## Opcode Categories

The full opcode listing is in [`vb6.md`](vb6.md). Below is a summary of all categories supported:

| Category | Count | Examples |
|----------|-------|---------|
| Control Flow | 68 | `Branch`, `BranchF`, `BranchT`, `ForI4`, `NextI4`, `ExitProc`, `GoSub`, `Return`, `OnErrorGoto`, `Resume` |
| Calls | 56 | `VCallHresult`, `ThisVCall`, `ImpAdCallI4`, `LateIdCall`, `RaiseEvent` |
| Load / Store | 267 | `FLdI2`, `FStVar`, `FMemLdR8`, `MemStStr`, `ImpAdLdI4`, `LitI4`, `LitStr`, `PopAd` |
| Arithmetic | 44 | `AddI4`, `SubR8`, `MulCy`, `DivR8`, `IDvI4`, `ModI4`, `PwrR8R8`, `UMiI4` |
| Logic / Bitwise | 19 | `AndI4`, `OrI4`, `XorI4`, `NotI4`, `EqvI4`, `ImpI4` |
| Comparison | 123 | `EqI4`, `NeStr`, `LtR8`, `GeVar`, `LikeStr`, `BetweenI4`, `Is` |
| Type Conversion | 124 | `CI2I4`, `CI4R8`, `CStrVar`, `CVarI4`, `CBoolVar`, `CUI1I2` |
| String Operations | 17 | `ConcatStr`, `MidStr`, `FnLenStr`, `FnInStr4`, `FnStrComp3` |
| Array Operations | 55 | `Ary1LdI4`, `Ary1StVar`, `ReDim`, `RedimPreserve`, `Erase`, `AryLock` |
| Built-in Functions | 19 | `FnAbsI4`, `FnSgnR8`, `FnFixR8`, `FnIntCy`, `FnLBound`, `FnUBound` |
| Object / COM | 33 | `New`, `CheckType`, `AddRef`, `SetVarVar`, `CastAd` |
| File I/O | 43 | `OpenFile`, `Close`, `Input`, `PrintChan`, `GetRec4`, `PutRec4`, `SeekFile` |
| Late Binding | 14 | `LateIdLdVar`, `LateMemSt`, `VarLateMemLdVar` |
| Miscellaneous | 15 | `Bos`, `SelectCaseByte`, `Assert`, `Error`, `CopyBytes` |

---

## Detection

PVDasm detects VB5 P-Code binaries by scanning the PE import table for `MSVBVM50.DLL`, then follows the `VBHeader -> ProjectData -> ObjectTable` chain to locate the P-Code region. The VB header begins with the `VB5!` magic signature (`0x21354256`). If the header chain cannot be parsed, it falls back to decoding the first executable PE section.

## Summary

- **1346 opcode entries** across 6 tables (1 base + 5 extended)
- **545 unique mnemonics** covering the full VB5 P-Code instruction set
- Unrecognized opcodes display as `db XX` (raw hex)
- Identical P-Code ISA to VB6 — see [`vb6.md`](vb6.md) for the complete per-opcode reference
