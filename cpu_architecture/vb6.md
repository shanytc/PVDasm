# PVDasm Visual Basic 6 P-Code Opcode Coverage

This document lists all VB6 P-Code opcodes supported by PVDasm. Every entry below has been verified against the source code in `VBPCode.cpp` and the auto-generated opcode table `VBOpcodeTable.h` (sourced from `vb_opcodes.json`).

P-Code is the bytecode produced when a VB6 project is compiled in "P-Code" mode (as opposed to Native Code). The VB6 runtime (`MSVBVM60.DLL`) interprets it at run-time.

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
- Arguments are encoded as type-tagged inline operands (BYTE, WORD, DWORD, signed offsets, etc.)

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

## Control Flow

Branching, looping, subroutine entry/exit, and error handling.

| Mnemonic | Description |
|----------|-------------|
| `Branch` | Unconditional branch (signed WORD offset) |
| `BranchF` | Branch if False |
| `BranchT` | Branch if True |
| `BranchFVar` | Branch if Variant is False |
| `BranchFVarFree` | Branch if Variant is False, free Variant |
| `BranchTVar` | Branch if Variant is True |
| `BranchTVarFree` | Branch if Variant is True, free Variant |
| `ForI2` / `ForI4` / `ForR4` / `ForR8` / `ForCy` / `ForUI1` / `ForVar` | For loop init (typed) |
| `ForStepI2` / `ForStepI4` / `ForStepR4` / `ForStepR8` / `ForStepCy` / `ForStepUI1` / `ForStepVar` | For..Step loop init (typed) |
| `ForEachAryVar` / `ForEachCollAd` / `ForEachCollObj` / `ForEachCollVar` / `ForEachVar` / `ForEachVarFree` | For Each loop init |
| `NextI2` / `NextI4` / `NextR8` / `NextUI1` | Next (loop increment, typed) |
| `NextStepI2` / `NextStepI4` / `NextStepR4` / `NextStepR8` / `NextStepCy` / `NextStepUI1` / `NextStepVar` | Next Step (custom increment) |
| `NextEachAryVar` / `NextEachCollAd` / `NextEachCollObj` / `NextEachCollVar` / `NextEachVar` | Next for For Each |
| `ExitForAryVar` / `ExitForCollObj` / `ExitForVar` | Exit For |
| `ExitProc` / `ExitProcHresult` / `ExitProcI2` / `ExitProcR4` / `ExitProcR8` / `ExitProcCy` / `ExitProcStr` / `ExitProcUI1` | Exit Sub/Function (typed return) |
| `ExitProcCb` / `ExitProcCbHresult` / `ExitProcCbStack` | Exit with callback cleanup |
| `ExitProcFrameCb` / `ExitProcFrameCbHresult` / `ExitProcFrameCbStack` | Exit with frame + callback cleanup |
| `GoSub` | GoSub (intra-procedure branch) |
| `Return` | Return from GoSub |
| `OnGoto` | On N GoTo dispatch |
| `OnGosub` | On N GoSub dispatch |
| `OnErrorGoto` | On Error GoTo handler |
| `Resume` | Resume (error handler) |
| `End` | End statement (terminate program) |
| `Stop` | Stop statement (break into debugger) |

---

## Calls

Method calls, virtual calls, and imported function calls.

| Mnemonic | Description |
|----------|-------------|
| `VCallAd` / `VCallHresult` / `VCallR4` / `VCallR8` / `VCallCy` / `VCallStr` / `VCallUI1` / `VCallFPR8` | Virtual method call via vtable (typed return) |
| `VCallCbFrame` | Virtual call with callback frame |
| `ThisVCall` / `ThisVCallAd` / `ThisVCallHresult` / `ThisVCallI2` / `ThisVCallR4` / `ThisVCallR8` / `ThisVCallCy` / `ThisVCallUI1` | Virtual call on Me/this (typed return) |
| `ThisVCallCbFrame` / `ThisVCallHidden` | This-call with callback frame / hidden return |
| `ImpAdCallHresult` / `ImpAdCallI2` / `ImpAdCallI4` / `ImpAdCallR4` / `ImpAdCallR8` / `ImpAdCallCy` / `ImpAdCallFPR4` / `ImpAdCallUI1` | Imported (DLL) function call (typed return) |
| `ImpAdCallCbFrame` / `ImpAdCallNonVirt` | Imported call with callback frame / non-virtual |
| `LateIdCall` / `LateIdCallLdVar` / `LateIdCallSt` | Late-bound call by ID |
| `LateIdNamedCall` / `LateIdNamedCallLdVar` / `LateIdNamedCallSt` | Late-bound call by named ID |
| `LateMemCall` / `LateMemCallLdVar` / `LateMemCallSt` | Late-bound member call |
| `LateMemNamedCall` / `LateMemNamedCallLdVar` / `LateMemNamedCallSt` | Late-bound named member call |
| `VarLateMemCallLdRfVar` / `VarLateMemCallLdVar` / `VarLateMemCallSt` | Variant late member call |
| `RaiseEvent` | RaiseEvent statement |

---

## Load / Store

Stack loads, stores, frame/member/import access, and literal constants.

### Literals

| Mnemonic | Description |
|----------|-------------|
| `LitI2` / `LitI2_Byte` / `LitI2FP` | Push Integer literal |
| `LitI4` | Push Long literal |
| `LitR4FP` / `LitR8` / `LitR8FP` | Push Single/Double literal |
| `LitCy` / `LitCy4` | Push Currency literal |
| `LitDate` | Push Date literal |
| `LitStr` | Push String literal (pointer to string pool) |
| `LitNothing` | Push Nothing (null object) |
| `LitVarI2` / `LitVarI4` / `LitVarR4` / `LitVarR8` / `LitVarCy` / `LitVarDate` / `LitVarStr` / `LitVarUI1` | Push typed Variant literal |
| `LitVar_Empty` / `LitVar_NULL` / `LitVar_Missing` / `LitVar_TRUE` / `LitVar_FALSE` | Push special Variant constants |

### Frame (Local) Load / Store — `FLd*` / `FSt*`

| Mnemonic | Description |
|----------|-------------|
| `FLdI2` / `FLdR4` / `FLdR8` / `FLdUI1` / `FLdVar` / `FLdFPR4` / `FLdFPR8` | Load local variable onto stack |
| `FLdPr` / `FLdPrThis` | Load local object pointer / Me |
| `FLdRfVar` | Load local Variant by reference |
| `FLdZeroAd` / `FLdZeroAry` | Load zero address / zero array |
| `FLdLateIdUnkVar` | Load late-bound unknown Variant |
| `FStI2` / `FStR4` / `FStR8` / `FStUI1` / `FStFPR4` / `FStFPR8` | Store to local (typed) |
| `FStStr` / `FStStrCopy` / `FStStrNoPop` | Store String to local |
| `FStVar` / `FStVarCopy` / `FStVarNoPop` / `FStVarZero` | Store Variant to local |
| `FStAd` / `FStAdFunc` / `FStAdFuncNoPop` / `FStAdNoPop` | Store address to local |
| `FStVarAd` / `FStVarAdFunc` / `FStVarCopyObj` / `FStVarUnk` / `FStVarUnkFunc` | Store Variant (special) |
| `FFree1Ad` / `FFree1Str` / `FFree1Var` | Free single local (address/String/Variant) |
| `FFreeAd` / `FFreeStr` / `FFreeVar` | Free multiple locals |
| `FreeAdNoPop` / `FreeStrNoPop` / `FreeVarNoPop` | Free without popping |
| `FDupStr` / `FDupVar` | Duplicate String/Variant on stack |

### Instance Member Load / Store — `FMem*`

| Mnemonic | Description |
|----------|-------------|
| `FMemLdI2` / `FMemLdR4` / `FMemLdR8` / `FMemLdCy` / `FMemLdFPR4` / `FMemLdFPR8` | Load member field (typed) |
| `FMemLdPr` / `FMemLdRf` / `FMemLdVar` | Load member pointer/reference/Variant |
| `FMemStI2` / `FMemStI4` / `FMemStR4` / `FMemStR8` / `FMemStUI1` / `FMemStFPR4` / `FMemStFPR8` | Store to member (typed) |
| `FMemStStr` / `FMemStStrCopy` / `FMemStAd` / `FMemStAdFunc` | Store String/address to member |
| `FMemStVar` / `FMemStVarAd` / `FMemStVarAdFunc` / `FMemStVarCopy` / `FMemStVarUnk` / `FMemStVarUnkFunc` | Store Variant to member |

### Indirect (With-block) Member — `IWMem*` / `WMem*`

| Mnemonic | Description |
|----------|-------------|
| `IWMemLdI2` / `IWMemLdI4` / `IWMemLdCy` / `IWMemLdFPR4` / `IWMemLdFPR8` / `IWMemLdUI1` | Indirect With-member load (typed) |
| `IWMemLdPr` / `IWMemLdRf` / `IWMemLdDarg` / `IWMemLdRfDarg` | Indirect With-member load (reference) |
| `IWMemStI2` / `IWMemStR4` / `IWMemStCy` / `IWMemStFPR4` / `IWMemStFPR8` / `IWMemStUI1` | Indirect With-member store (typed) |
| `IWMemStStr` / `IWMemStStrCopy` / `IWMemStAd` / `IWMemStAdFunc` | Indirect With-member store (String/address) |
| `IWMemStDarg` / `IWMemStDargAd` / `IWMemStDargAdFunc` / `IWMemStDargCopy` / `IWMemStDargUnk` / `IWMemStDargUnkFunc` | Indirect With-member store (Darg) |
| `WMemLdI2` / `WMemLdCy` / `WMemLdFPR4` / `WMemLdFPR8` / `WMemLdUI1` / `WMemLdStr` / `WMemLdVar` / `WMemLdRfVar` | With-member load |
| `WMemStI2` / `WMemStR4` / `WMemStR8` / `WMemStCy` / `WMemStFPR4` / `WMemStFPR8` / `WMemStUI1` | With-member store (typed) |
| `WMemStStr` / `WMemStStrCopy` / `WMemStAd` / `WMemStAdFunc` | With-member store (String/address) |
| `WMemStVar` / `WMemStVarAd` / `WMemStVarAdFunc` / `WMemStVarCopy` / `WMemStVarUnk` / `WMemStVarUnkFunc` | With-member store (Variant) |

### Module-level Member — `Mem*`

| Mnemonic | Description |
|----------|-------------|
| `MemLdI2` / `MemLdR4` / `MemLdR8` / `MemLdFPR4` / `MemLdFPR8` / `MemLdUI1` / `MemLdStr` / `MemLdVar` | Module member load |
| `MemLdPr` / `MemLdRfVar` | Module member pointer/reference load |
| `MemStI2` / `MemStI4` / `MemStR4` / `MemStR8` / `MemStCy` / `MemStFPR4` / `MemStFPR8` / `MemStUI1` | Module member store (typed) |
| `MemStStr` / `MemStStrCopy` / `MemStAd` / `MemStAdFunc` | Module member store (String/address) |
| `MemStVar` / `MemStVarAd` / `MemStVarAdFunc` / `MemStVarCopy` / `MemStVarUnk` / `MemStVarUnkFunc` | Module member store (Variant) |

### Imported Address — `ImpAd*`

| Mnemonic | Description |
|----------|-------------|
| `ImpAdLdI2` / `ImpAdLdI4` / `ImpAdLdCy` / `ImpAdLdFPR4` / `ImpAdLdFPR8` / `ImpAdLdUI1` / `ImpAdLdStr` / `ImpAdLdVar` | Load from imported address |
| `ImpAdLdPr` / `ImpAdLdRf` | Load pointer/reference from imported address |
| `ImpAdStI2` / `ImpAdStR4` / `ImpAdStR8` / `ImpAdStCy` / `ImpAdStFPR4` / `ImpAdStFPR8` / `ImpAdStUI1` / `ImpAdStStr` / `ImpAdStStrCopy` | Store to imported address |
| `ImpAdStAd` / `ImpAdStAdFunc` | Store address to imported address |
| `ImpAdStVar` / `ImpAdStVarAd` / `ImpAdStVarAdFunc` / `ImpAdStVarCopy` / `ImpAdStVarUnk` / `ImpAdStVarUnkFunc` | Store Variant to imported address |

### Indirect (I-prefix) Load / Store

| Mnemonic | Description |
|----------|-------------|
| `ILdI2` / `ILdI4` / `ILdR8` / `ILdFPR4` / `ILdFPR8` / `ILdUI1` / `ILdAd` | Indirect load (typed) |
| `ILdPr` / `ILdRf` / `ILdDarg` / `ILdRfDarg` | Indirect load (pointer/reference/Darg) |
| `IStI2` / `IStI4` / `IStR8` / `IStFPR4` / `IStFPR8` / `IStUI1` | Indirect store (typed) |
| `IStStr` / `IStStrCopy` / `IStAd` / `IStAdFunc` | Indirect store (String/address) |
| `IStDarg` / `IStDargAd` / `IStDargAdFunc` / `IStDargCopy` / `IStDargUnk` / `IStDargUnkFunc` | Indirect store (Darg) |
| `IStVarCopyObj` | Indirect store Variant (object copy) |

### Stack / Temp Operations

| Mnemonic | Description |
|----------|-------------|
| `PopAd` | Pop address from stack |
| `PopAdLd4` / `PopAdLdVar` | Pop address and load 4-byte/Variant |
| `PopFPR4` / `PopFPR8` | Pop FP value from FPU stack |
| `PopTmpLdAd1` / `PopTmpLdAd2` / `PopTmpLdAd8` | Pop to temp, load address |
| `PopTmpLdAdFPR4` / `PopTmpLdAdFPR8` / `PopTmpLdAdStr` / `PopTmpLdAdVar` | Pop to temp, load typed address |
| `LdFrameRf` / `LdPrVar` / `LdPrUnkVar` / `LdStkRf` / `LdFixedStr` | Load frame ref / stack ref / fixed string |

### Fixed String / Record Store

| Mnemonic | Description |
|----------|-------------|
| `StFixedStr` / `StFixedStrFree` / `StFixedStrR` / `StFixedStrRFree` | Store to fixed-length string |
| `StLsetFixStr` | LSet fixed string |
| `StUdtVar` | Store UDT Variant |
| `StAryMove` / `StAryCopy` / `StAryRecCopy` / `StAryRecMove` / `StAryVar` | Store array (move/copy) |

---

## Arithmetic

| Mnemonic | Description |
|----------|-------------|
| `AddI2` / `AddI4` / `AddR4` / `AddR8` / `AddCy` / `AddCyR8` / `AddUI1` / `AddVar` | Addition (typed) |
| `SubI2` / `SubI4` / `SubR4` / `SubCy` / `SubUI1` / `SubVar` | Subtraction (typed) |
| `MulI2` / `MulI4` / `MulR4` / `MulR8` / `MulCy` / `MulCyI2` / `MulUI1` / `MulVar` | Multiplication (typed) |
| `DivR8` / `DivVar` | Floating-point division |
| `IDvI2` / `IDvI4` / `IDvUI1` / `IDvVar` | Integer division (`\`) |
| `ModI2` / `ModI4` / `ModUI1` / `ModVar` | Modulo |
| `PwrR8I2` / `PwrR8R8` / `PwrVar` | Exponentiation (`^`) |
| `UMiI2` / `UMiI4` / `UMiR4` / `UMiR8` / `UMiCy` / `UMiVar` | Unary minus (negation) |

---

## Logic / Bitwise

| Mnemonic | Description |
|----------|-------------|
| `AndI4` / `AndUI1` / `AndVar` | Bitwise AND |
| `OrI2` / `OrI4` / `OrVar` | Bitwise OR |
| `XorI4` / `XorVar` | Bitwise XOR |
| `NotI4` / `NotUI1` / `NotVar` | Bitwise NOT |
| `EqvI4` / `EqvUI1` / `EqvVar` | Bitwise EQV (equivalence) |
| `ImpI4` / `ImpUI1` / `ImpVar` | Bitwise IMP (implication) |

---

## Comparison

All comparison opcodes push a Boolean result onto the stack.

| Mnemonic | Description |
|----------|-------------|
| `EqI2` / `EqI4` / `EqR4` / `EqR8` / `EqCy` / `EqCyR8` / `EqStr` / `EqVar` / `EqVarBool` | Equal (`=`) |
| `EqTextStr` / `EqTextVar` / `EqTextVarBool` | Equal (text/case-insensitive) |
| `NeI2` / `NeI4` / `NeR4` / `NeR8` / `NeCy` / `NeCyR8` / `NeStr` / `NeVar` / `NeVarBool` | Not equal (`<>`) |
| `NeTextStr` / `NeTextVar` / `NeTextVarBool` | Not equal (text) |
| `LtI2` / `LtI4` / `LtR4` / `LtR8` / `LtCy` / `LtCyR8` / `LtStr` / `LtUI1` / `LtVar` / `LtVarBool` | Less than (`<`) |
| `LtTextStr` / `LtTextVar` / `LtTextVarBool` | Less than (text) |
| `LeI2` / `LeI4` / `LeR4` / `LeR8` / `LeCy` / `LeCyR8` / `LeStr` / `LeUI1` / `LeVar` / `LeVarBool` | Less or equal (`<=`) |
| `LeTextStr` / `LeTextVar` / `LeTextVarBool` | Less or equal (text) |
| `GtI2` / `GtI4` / `GtR4` / `GtR8` / `GtCy` / `GtCyR8` / `GtStr` / `GtUI1` / `GtVar` / `GtVarBool` | Greater than (`>`) |
| `GtTextStr` / `GtTextVar` / `GtTextVarBool` | Greater than (text) |
| `GeI2` / `GeI4` / `GeR4` / `GeR8` / `GeCy` / `GeCyR8` / `GeStr` / `GeUI1` / `GeVar` / `GeVarBool` | Greater or equal (`>=`) |
| `GeTextStr` / `GeTextVar` / `GeTextVarBool` | Greater or equal (text) |
| `LikeStr` / `LikeVar` / `LikeVarBool` | Like pattern match |
| `LikeTextStr` / `LikeTextVar` / `LikeTextVarBool` | Like pattern match (text) |
| `BetweenI2` / `BetweenI4` / `BetweenR4` / `BetweenCy` / `BetweenUI1` / `BetweenStr` / `BetweenVar` | Between range test |
| `BetweenTextStr` / `BetweenTextVar` | Between range test (text) |
| `Is` | Object identity (`Is` operator) |

---

## Type Conversion

| Mnemonic | Description |
|----------|-------------|
| `CI2I4` / `CI2R8` / `CI2Cy` / `CI2Str` / `CI2UI1` / `CI2Var` | Convert to Integer |
| `CI4I2` / `CI4R8` / `CI4Cy` / `CI4Str` / `CI4UI1` / `CI4Var` | Convert to Long |
| `CR4R4` / `CR4R8` / `CR4I4` / `CR4Str` / `CR4Var` | Convert to Single |
| `CR8I2` / `CR8I4` / `CR8R4` / `CR8R8` / `CR8Cy` / `CR8Str` / `CR8Var` | Convert to Double |
| `CCyI2` / `CCyI4` / `CCyR4` / `CCyStr` / `CCyVar` | Convert to Currency |
| `CUI1I2` / `CUI1I4` / `CUI1R4` / `CUI1Bool` / `CUI1Cy` / `CUI1Str` / `CUI1Var` | Convert to Byte |
| `CBoolI4` / `CBoolR4` / `CBoolCy` / `CBoolStr` / `CBoolUI1` / `CBoolVar` / `CBoolVarNull` | Convert to Boolean |
| `CDateR8` / `CDateStr` | Convert to Date |
| `CStrI2` / `CStrI4` / `CStrR4` / `CStrR8` / `CStrCy` / `CStrBool` / `CStrDate` / `CStrUI1` / `CStrVar` / `CStrVarTmp` / `CStrVarVal` | Convert to String |
| `CStr2Ansi` / `CStr2Uni` / `CStr2Vec` / `CStrAry2Ansi` / `CStrAry2Uni` | String encoding conversion |
| `CVarI2` / `CVarI4` / `CVarR4` / `CVarR8` / `CVarCy` / `CVarDate` / `CVarStr` / `CVarUI1` / `CVarUnk` / `CVarAd` | Convert to Variant |
| `CVarBoolI2` / `CVarErrI4` / `CVarDateVar` / `CVarRef` / `CVarRefUdt` / `CVarAryUdt` / `CVarAryVarg` / `CVarUdt` | Convert to Variant (special) |
| `CVar2Vec` / `CVec2Var` / `CRefVarAry` / `CUnkVar` / `CAdVar` | Variant/Vector/Unknown conversion |
| `CDargRef` / `CDargRefUdt` | Convert Darg reference |
| `CRec2Ansi` / `CRec2Uni` / `CRecAnsi2Uni` / `CRecUni2Ansi` | Record ANSI/Unicode conversion |
| `FnCBoolVar` / `FnCByteVar` / `FnCCurVar` / `FnCDblCy` / `FnCDblR8` / `FnCDblVar` | Built-in CBool/CByte/CCur/CDbl |
| `FnCDateVar` / `FnCIntVar` / `FnCLngVar` / `FnCSngCy` / `FnCSngI2` / `FnCSngI4` / `FnCSngStr` / `FnCSngVar` / `FnCStrVar` | Built-in CDate/CInt/CLng/CSng/CStr |

---

## String Operations

| Mnemonic | Description |
|----------|-------------|
| `ConcatStr` / `ConcatVar` | String concatenation (`&`) |
| `MidStr` / `MidVar` | Mid$ assignment (`Mid$(s,n) = x`) |
| `MidBStr` / `MidBStrB` / `MidBVar` | MidB$ / MidB assignment |
| `FnLenStr` / `FnLenVar` | Len() function |
| `FnLenBStr` / `FnLenBVar` | LenB() function |
| `FnInStr4` / `FnInStr4Var` | InStr() function |
| `FnInStrB4` / `FnInStrB4Var` | InStrB() function |
| `FnStrComp3` / `FnStrComp3Var` | StrComp() function |

---

## Array Operations

| Mnemonic | Description |
|----------|-------------|
| `Ary1LdI2` / `Ary1LdI4` / `Ary1LdCy` / `Ary1LdR8` / `Ary1LdFPR4` / `Ary1LdFPR8` / `Ary1LdUI1` / `Ary1LdVar` / `Ary1LdVarg` | 1-D array element load (typed) |
| `Ary1LdPr` / `Ary1LdRf` / `Ary1LdRfVar` / `Ary1LdRfVarg` / `Ary1LdRfVargParam` | 1-D array element load (reference) |
| `Ary1StI2` / `Ary1StI4` / `Ary1StR4` / `Ary1StCy` / `Ary1StFPR4` / `Ary1StFPR8` / `Ary1StUI1` / `Ary1StStr` / `Ary1StStrCopy` / `Ary1StVar` | 1-D array element store (typed) |
| `Ary1StAd` / `Ary1StAdFunc` / `Ary1StVarAd` / `Ary1StVarAdFunc` / `Ary1StVarCopy` / `Ary1StVarUnk` / `Ary1StVarUnkFunc` | 1-D array element store (special) |
| `Ary1StVarg` / `Ary1StVargAd` / `Ary1StVargAdFunc` / `Ary1StVargCopy` / `Ary1StVargUnk` / `Ary1StVargUnkFunc` | 1-D ParamArray element store |
| `ParmAry1St` | Parameter array store |
| `AryLdPr` / `AryLdRf` | Multi-D array load (pointer/reference) |
| `AryDescTemp` | Create temp array descriptor |
| `AryInRecLdPr` / `AryInRecLdRf` | Array-in-record load |
| `AryLock` / `AryUnlock` | Lock/unlock array (SafeArrayLock/Unlock) |
| `ArrayRebase1Var` | Rebase 1-D Variant array |
| `Erase` / `EraseNoPop` / `EraseDestruct` / `EraseDestrKeepData` | Erase array |
| `ReDim` / `RedimVar` / `RedimVarUdt` | ReDim array |
| `RedimPreserve` / `RedimPreserveVar` / `RedimPreserveVarUdt` | ReDim Preserve array |

---

## Built-in Functions

| Mnemonic | Description |
|----------|-------------|
| `FnAbsI2` / `FnAbsI4` / `FnAbsR4` / `FnAbsR8` / `FnAbsCy` / `FnAbsVar` | Abs() |
| `FnSgnI4` / `FnSgnR4` / `FnSgnR8` / `FnSgnCy` / `FnSgnUI1` | Sgn() |
| `FnFixR8` / `FnFixCy` / `FnFixVar` | Fix() |
| `FnIntR8` / `FnIntCy` / `FnIntVar` | Int() |
| `FnLBound` | LBound() |
| `FnUBound` | UBound() |

---

## Object / COM

| Mnemonic | Description |
|----------|-------------|
| `New` | New object instantiation |
| `NewIfNullPr` / `NewIfNullAd` / `NewIfNullRf` | Create object if reference is Nothing |
| `CheckType` / `CheckTypeVar` | TypeOf...Is check |
| `CastAd` / `CastAdVar` | Cast address / Variant address |
| `AddRef` | AddRef (COM reference count increment) |
| `SetVarVar` / `SetVarVarFunc` | Set object reference |
| `VerifyPVarObj` / `VerifyVarObj` | Verify Variant contains valid object |
| `VarIndexLdRfVar` / `VarIndexLdRfVarLock` / `VarIndexLdVar` | Variant index load |
| `VarIndexSt` / `VarIndexStAd` | Variant index store |
| `GetLastError` / `SetLastSystemError` | Win32 last error |
| `CExtInstUnk` | Create external instance (unknown) |

---

## File I/O

| Mnemonic | Description |
|----------|-------------|
| `OpenFile` / `Close` / `CloseAll` | Open / Close file |
| `Input` / `InputDone` | Input# statement |
| `InputItemI2` / `InputItemI4` / `InputItemR4` / `InputItemR8` / `InputItemCy` / `InputItemDate` / `InputItemStr` / `InputItemUI1` / `InputItemBool` / `InputItemVar` | Input# typed item |
| `LineInputStr` / `LineInputVar` | Line Input# |
| `PrintChan` / `PrintFile` / `PrintObj` / `PrintObject` | Print# targets |
| `PrintComma` / `PrintItemComma` / `PrintItemNL` / `PrintItemSemi` / `PrintNL` / `PrintEos` | Print# formatting |
| `PrintSpc` / `PrintTab` | Print# Spc/Tab functions |
| `WriteChan` / `WriteFile` | Write# statement |
| `GetRec3` / `GetRec4` / `GetRecFxStr3` / `GetRecFxStr4` | Get# (random access read) |
| `GetRecOwn3` / `GetRecOwn4` / `GetRecOwner3` / `GetRecOwner4` | Get# (owner variants) |
| `PutRec3` / `PutRec4` / `PutRecFxStr3` / `PutRecFxStr4` | Put# (random access write) |
| `PutRecOwn3` / `PutRecOwn4` / `PutRecOwner3` / `PutRecOwner4` | Put# (owner variants) |
| `SeekFile` / `LockFile` / `NameFile` | Seek / Lock / Name |

---

## Late Binding (IDispatch)

| Mnemonic | Description |
|----------|-------------|
| `LateIdLdVar` / `LateIdSt` / `LateIdStAd` | Late-bound property Get/Let/Set by DispID |
| `LateIdNamedStAd` | Late-bound named property Set by DispID |
| `LateMemLdVar` / `LateMemSt` / `LateMemStAd` | Late-bound member property Get/Let/Set |
| `LateMemNamedStAd` | Late-bound named member property Set |
| `VarLateMemLdRfVar` / `VarLateMemLdVar` / `VarLateMemSt` / `VarLateMemStAd` | Variant late-bound member access |

---

## Miscellaneous

| Mnemonic | Description |
|----------|-------------|
| `Bos` / `LargeBos` | Beginning-of-statement marker |
| `DOC` | Documentation / placeholder (no operation) |
| `SelectCaseByte` | Select Case dispatch byte |
| `HardType` | Hard type assertion |
| `Assert` | Debug.Assert |
| `Error` | Error statement (raise error) |
| `InvalidExcode` | Invalid execution code marker |
| `CopyBytes` / `CopyBytesZero` | Memory copy / zero |
| `AssignRecord` | Assign UDT record |
| `DestructOFrame` / `DestructAnsiOFrame` / `DestructRecord` | Destruct object frame / record |
| `ZeroRetVal` / `ZeroRetValVar` | Zero the return value |

---

## Detection

PVDasm detects VB6 P-Code binaries by scanning the PE import table for `MSVBVM60.DLL`, then follows the `VBHeader -> ProjectData -> ObjectTable` chain to locate the P-Code region. If the header chain cannot be parsed, it falls back to decoding the first executable PE section.

## Summary

- **1346 opcode entries** across 6 tables (1 base + 5 extended)
- **545 unique mnemonics** covering the full VB6 P-Code instruction set
- Unrecognized opcodes display as `db XX` (raw hex)
- The VB5 and VB6 P-Code instruction sets are identical; only the runtime DLL differs (`MSVBVM50.DLL` vs `MSVBVM60.DLL`)
