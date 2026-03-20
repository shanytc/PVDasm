<p align="left">
  <pre align="center">
 /$$$$$$$  /$$    /$$ /$$$$$$$   /$$$$$$   /$$$$$$  /$$      /$$
| $$__  $$| $$   | $$| $$__  $$ /$$__  $$ /$$__  $$| $$$    /$$$
| $$  \ $$| $$   | $$| $$  \ $$| $$  \ $$| $$  \__/| $$$$  /$$$$
| $$$$$$$/|  $$ / $$/| $$  | $$| $$$$$$$$|  $$$$$$ | $$ $$/$$ $$
| $$____/  \  $$ $$/ | $$  | $$| $$__  $$ \____  $$| $$  $$$| $$
| $$        \  $$$/  | $$  | $$| $$  | $$ /$$  \ $$| $$\  $ | $$
| $$         \  $/   | $$$$$$$/| $$  | $$|  $$$$$$/| $$ \/  | $$
|__/          \_/    |_______/ |__/  |__/ \______/ |__/     |__/
</pre>
</p>

<p align="center">
  <strong>32/64-Bit Multi Disassembler</strong><br>
  Build Version: <code>v1.9e</code><br>
  &copy; Shany Golan 2003&ndash;2026
</p>

<p align="center">
  <a href="#general-overview">Overview</a> &bull;
  <a href="#features">Features</a> &bull;
  <a href="#changelog">Changelog</a> &bull;
  <a href="https://grokipedia.com/page/PVDasm" target="_blank">Grokipedia</a>
</p>

---

## General Overview

**PVDasm** (ProView) is a fully hand-written disassembler, built from scratch. The disassembler engine was developed independently and is **free for anyone to use** and has been integrated into PVDAsm.

ProView is an educational project aimed at building a custom disassembler and deepening CPU architecture knowledge. It is written entirely in **C** (IDE: VC++ 6, VS 2005/2010/2017/2022), **C++**, and **STL Templates**.

## Features

| Feature | Description |
|---|---|
| **Disassembler Engine** | Custom x86 engine with MMX, 3DNow!, SSE, and SSE2 support and more. |
| **PE/PE+ Editor** | View and edit PE (32-bit) and PE+ (64-bit) executable headers + Rebuild Tools. |
| **Dark Mode** | Light/Dark mode UI support. |
| **Function Analysis** | Prologue/epilogue detection, CALL target recognition, FPO prologues, parameters, and local variables. Searchable function list (`Ctrl+J`). |
| **Process Manager** | View and dump running processes (Win2K/XP/7+) |
| **Plugin SDK** | Develop and load custom plugins from the `\Plugin\` directory. |
| **MASM Wizard** | Generate MASM source code from disassembled output. |
| **Code Patcher** | Inline assembly patcher with live preview. |
| **Branch Tracing** | Trace jumps/calls and return with arrow keys. |
| **XReferences** | Cross-reference viewer with search. |
| **String References** | String reference browser with search dialog. |
| **HexEditor** | Embedded hex editor (RadASM add-in.) |
| **PVScript Engine** | Custom scripting engine for command automation. |
| **API Recognition** | Automatic API parameter annotation via signature database. |
| **Function Graph (CFG)** | Interactive control flow graph viewer with draggable blocks, zoom/pan, orthogonal edge routing, instruction-level navigation (double-click jxx/call), call history with back-navigation, and export to PNG/JPEG. |
| **Color Schemes** | SoftICE, IDA, OllyDbg, W32Asm, and custom themes. |
| **Save/Load Projects** | Persist analysis state including functions, data, and comments. |
| **Map File Support** | Import/export MAP files (IDA-compatible.) |
| **x86-64 Disassembly** | Full AMD64 long mode support for PE+ (64-bit) executables. |
| **Multi CPU Support** | Support Intel x86, x86-64, Chip8, Visual Basic architectures. |
---

## Changelog

### 2026

#### March 20, 2026
- Added **CFG instruction-level navigation**: double-click a `jxx` line to pan to its target block, double-click a `call` line to rebuild the CFG for the called function
- Added **CFG call history**: navigate into called functions and press `Backspace` to return to the previous function's graph (up to 16 levels)
- Added **CFG context menu**: "Goto Start" (entry block), "Goto End" (exit block), and "Goto Caller" (conditional predecessor block)
- Added **hand cursor** on hover over navigable `jxx`/`call` instructions in the CFG viewer
- Added **Show Functions dialog** (`Ctrl+J`) — searchable list of all detected functions with click-to-navigate
- Improved **function detection**: CALL target recognition via E8 pre-scan, FPO prologue detection (`SUB ESP/RSP`), false positive suppression
- Fixed prologue comments ("save frame pointer") only appearing at actual function entries, not on stray `PUSH EBP` instructions
- Synced **code map** and **CFG viewer** with new function detection (CallTargets, heuristic prologues)
- Fixed **crash in String References** dialog (buffer overflow from `strcpy_s` misuse, unbounded `while` loop)
- Added **subroutine banners** for function prologues in disassembly
- Added **live selection highlight** and **middle-mouse pan** in code map
- Fixed **CFG viewer**: opens for the currently selected function (not always entry point), stops tracing at function boundaries, reuses window when switching functions, and handles entry point banner at index 0

#### March 18–19, 2026
- Implemented **VMX extended instructions**: VMREAD, VMWRITE, VMPTRLD, VMPTRST, VMCLEAR, VMXON, INVEPT, INVVPID
- Implemented remaining **AVX-512 DQ & BW** instruction gaps
- Added `INCSSPD`/`INCSSPQ` (F3 0F AE) prefix recognition
- Fixed `VCVTQQ2PS`/`VCVTQQ2PD` operand format (2-operand form, half-size destination)
- Added PVDasm vs IDA comparison scripts with `.lst` format support
- Added cpu_architecture docs for CHIP-8, VB5, and VB6 P-Code

#### March 15–16, 2026
- Added **x86-64 (AMD64) long mode disassembly** for PE+ executables
  - REX prefix decoding and 64-bit register promotion
  - RIP-relative addressing
  - Widened address column for 16-digit 64-bit addresses
  - Fixed EVEX fall-through, PE64 section headers, and REX register promotion
  - Fixed MOVSXD operand order, `CALL`/`JMP QWORD PTR`, and `JMP RAX` in 64-bit mode
  - Fixed goto entrypoint crash and auto-goto entrypoint on 64-bit PE files
  - Fixed buffer overflow and address truncation in jump/call navigation for 64-bit PE
- Fixed `MOVBE` memory operand decoding and `FNCLEX`/`FNINIT` mnemonics
- Added function prologue analysis in areas marked as data sections
- Fixed 6 x64 disassembly bugs found by comparing against IDA/objdump
- Bumped version to **v1.9d**

#### March 14, 2026
- Added **Function Graph (CFG) Viewer** — interactive control flow graph for functions
  - Orthogonal edge routing with port spreading and obstacle avoidance
  - Draggable blocks with edge re-routing on move
  - Color-coded blocks: green (true branch), red (false branch), blue (exit/return)
  - CALL edges with function name headers and `loc_` labels on jump targets
  - Comments displayed inline within blocks
  - Zoom/pan with mouse wheel and drag
  - Right-click context menu: "Fit Graph to Screen", "Save as Image..." (PNG with transparency / JPEG), "Goto Start/End/Caller"
  - Auto-fit graph on window resize
  - Double-click `jxx` to jump to target block, `call` to navigate into function (Backspace to go back), other lines to navigate in main disassembly

#### March 13, 2026
- Added/Fixed Architectures, PVDAsm now supports:
  - AVX
  - AVX2
  - AVX-512
  - Chip-8
  - MMX
  - SSE
  - SSE2
  - SSE3
  - SSE4
  - x86
  - x87 (FPU)
  - Visual Basic P-Code

#### March 12, 2026
- Refactored <a href="https://www.assembly.com.br/projects/HexEd.zip" target="_blank">HexEd</a> 32-bit MASM Project to 64Bit Masm using UASM.
  - Fixed hex editor crashes on 32-Bit and 64-Bit DLLs:
    - EDITSTREAM struct layout mismatch with RAHexEd DLL
    - DLL uses 8-byte aligned `pfnCallback` (offset 16, 24 bytes total) vs SDK `pack(4)` layout (offset 12, 20 bytes)
    - Added `RAHEX_EDITSTREAM` struct matching the DLL for both 32-bit and 64-bit builds
- Fixed `DWORD` to `DWORD_PTR` for stream callback handles (64-bit handle truncation)
- Fixed `strcpy_s` buffer overflow in hex editor file loading (`Temp[20]` to `Temp[MAX_PATH]`)
- Added `WS_VISIBLE` to RAHEXEDIT control in hex editor dialog resource

#### March 11, 2026
- Added Support for Visual Basic 5/6 Decompilation - Thanks @sp0tz for the PR.
- Refactored/Enhanced PE/PE+ rebuild capabilities:
  - Scylla/ImpREC like IAT builder for dumped processes

#### January 20, 2026
- Added **Dark Mode** support
- Added function prologue/epilogue, parameters, and local variables analysis
- Added search comments dialog and "find next" button to xref/imports dialogs
- Added hand cursor when hovering over jump instructions with jump-to-target in xref dialog
- Added double buffering for the list view to avoid visual artifacts
- Fixed disasm engine bug for `jnz` instruction address
- Fixed Task Manager and process dump issues
- Fixed UTF encoding issue
- Various UI improvements including live assembly preview in patch editor

### 2024

#### December 10, 2024
- Fixed a memory violation bug in the PE+ Editor

### 2022

#### December 9, 2022
- Fixed crash associated with subclassing edit text inputs on Windows 10/11
- Fixed LoadPic library to compile standalone

### 2015

#### April 1, 2015
- **PVDasm is now open sourced!**

### 2011

#### August 14, 2011
- Fixed ToolBar in PVDasm 64-bit (magic line: `TB_BUTTONSTRUCTSIZE`)
- Task Manager working on PVDasm 64-bit (disabled `VDMDBG.DLL`, uses `PSAPI.dll` only)
- HexEditor DLL requires x64 recompile by KetilO (RadASM project)

#### August 13, 2011
- Fixed status bar not showing Code Address / Code Offset on click
- Fixed crash when resolving API calls outside current disassembled section
- Fixed crash from DWORD/WORD memory access mismatch
- Increased buffer size in XRef resolving (stack runtime error fix)
- Fixed 64-bit import table resolve (`PIMAGE_THUNK_DATA64` vs `PIMAGE_THUNK_DATA32`)
- PE/PE+ Editor fixed in 64-bit build
- Menu bar re-ordered

> **Note:** PVDasm is now fully 32/64-bit compatible. Old 16-bit binaries load much safer.

#### March 27, 2011
- Fixed crash in PE Import scanner

### 2010

#### February 20, 2010
- Fixed buffer access violation in PE Imports scanner (thanks *SecMAM*)
- Increased buffer sizes for extra caution
- Changed `MoveWindow()` to `SetWindowPos()` for better compatibility
- Changed `SetWindowLong` to `SetWindowLongPtr` for 32/64-bit compatibility (thanks *jstorme*)

### 2009

#### March 28, 2009
- Fixed crash on re-disassembly of the same file

#### March 20, 2009
- Fixed corrupted resources (menu bar icons)

### 2008&ndash;2009
- GUI changes, addons, and regrouping
- 64-bit PVDasm compiled and tested under WinXP/Win7 64-bit
- PE+ Header (64-bit) supported in editor (no PE64 disassembly yet)

### 2006

#### November 8, 2006
- Added NE Executable file format loading support

#### November 4, 2006
- Added `ret` flag to the CodeFlow array and `DISASSEMBLY` struct

#### October 28, 2006
- Updated Plugin SDK with 3 new messages: `PI_GET_NUMBER_OF_EXPORTS`, `PI_GET_EXPORT_NAME`, `PI_GET_EXPORT_DASM_INDEX`
- Goto Address box now includes the image base

#### September 19, 2006
- **PVScript Engine v1** added &mdash; custom command scripting (`Ctrl+Enter` to execute)

#### May 12, 2006
- GUI changes

#### May 6, 2006
- PE+ (64-bit) executable editing support
- Fixed crash in MASM Wizard code editor

#### March 4, 2006
- Recoded copy to clipboard/file with dynamic memory allocation

### 2005

#### August 21, 2005
- Improved mouse hovering bug over `jxx`/`call` instructions during horizontal scroll

#### June 21, 2005
- PVDasm now handles read-only files (auto-removes read-only flag)

#### May 21, 2005
- Fixed decode bug: missing 2 bytes with `0x67` prefix instructions

#### April 23, 2005
- ToolTip helper window closes when mouse leaves the window area

#### April 8, 2005
- Bug fixes in function EntryPoint editor (now accepts only DWORD-length addresses)

#### March 24, 2005
- Fixed security buffer overflow in 2 places missing `MAX_PATH` (thanks for the report)

#### February 12, 2005
- Added File Map Export (`File -> Produce -> Map File`)
- Function name updates now reflect without re-disassembly
- Updated `IDA2PV.idc`

#### February 1, 2005
- Updated `IDA2PV.idc` to export function names
- MAP importing now reads function names from `.map` files

#### January 29, 2005
- MASM Wizard: string data references now use `PUSH OFFSET <MY_STR_NAME>`
- Added exception handlers, duplicate `.DATA` line remover, more default Libs/Incs
- Fixed newline bug with `0x0A` in Wizard output
- Fixed project save/load bug

#### January 25, 2005
- Fixed buffer overflow when reading imports
- Added more Plugin messages
- Save/Load project supports function names

#### January 22, 2005
- Functions can now be renamed
- MASM Wizard supports named functions (`Call "MyFunc"` instead of `Call XXXXXXXX`)

#### January 13, 2005
- Fixed disasm engine bugs reported by *mcoder*
- Added missing unhandled opcodes

### 2004

#### November 27, 2004
- Fixed invalid entrypoint pointer crash
- Fixed freeze when mouse leaves disasm client window (tooltip `mouse_move` event fix)

#### November 23, 2004
- Added 2 more Plugin messages
- Fixes reported by *elfZ*:
  - Corrected tab order in "Decode new file" dialog
  - Added keyboard navigation & `Esc` close for XRef window
  - Changed `Ctrl+C` from "go to code start" to "copy to clipboard"
  - Tab order fixes in all dialogs
  - More keyboard shortcuts added
  - Fixed typos, added "Add Comment" to menu
  - Added string display for `LEA` instructions (e.g., `ASCIIZ "My String"`)
- **ToolTip Helper**: hover over `CALL`/`JXX` addresses to preview target code
- **MAP Reader**: load IDA-generated MAP files via `Map/pvmap.idc`

#### August 26, 2004
- Added 6 more Plugin messages
- Function end markers: `RET ; proc_XXXXXXXX endp`
- "Close File" menu option added
- Fixed decoding of opcode sequence `8C3F`

#### July 25, 2004
- Fixed out-of-range pointer exception on large files (>300KB)
- Fixed disassembly bug: `[ESP+ESP+0Xh]` &rarr; `[ESP+0Xh]` (thanks *CoDe_InSide*)
- MMX-optimized `StringHex->Hex` conversion and `lstrlen()` functions
- 2 bug fixes in MASM Wizard (data string pointer and function entry point)
- Export/Import function database for MASM Wizard subroutine maker
- Added "Auto Search" button to MASM code builder
- Fixed crash on disassembly view save cancel

#### July 2, 2004
- **MASM Wizard enhanced:**
  - Cleans `LEAVE` instructions in procs
  - Fixes `RET XXXX` &rarr; `RET`
  - Auto-labels for jumps and calls (e.g., `jnz ref_12345678`, `call proc_87654321`)
  - Procedure stack frame cleanup (removes `PUSH EBP` / `MOV EBP,ESP` signatures)
- Added preceding zero for hex values (e.g., `0FFFFFFFFh`)

#### July 1, 2004
- Small disasm engine bug fix
- Save/Load project supports Data-In-Code & Function EntryPoint info
- Bug fixes in MASM Wizard
- Conditional/unconditional jump references shown in new disassembly column

#### March 21, 2004
- Fixed: `PUSH DWORD PTR SS:[EBP+00]` &rarr; `PUSH DWORD PTR SS:[EBP+EAX+00]` (thanks *_Seven_*)
- Added **MASM Source Code Wizard Creator**

#### February 14, 2004
- Double-click `CALL<API>` jumps to `JMP<API>` table
- **API Recognition engine** added (signature database: `\sig\msapi.sig`)
- Define address range as Data (`Ctrl+Insert`) or Function EntryPoint (`Alt+Insert`)
- Data/Function managers accessible via context menu

#### February 10, 2004
- New Plugin message added
- Updated Help file
- Function start/end address editor for FirstPass analysis

#### January 26, 2004
- API Calls/Jumps highlighting with customizable colors
- Right-click context menu on disasm window
- Copy to File/Clipboard via context menu

### 2003

#### December 15, 2003
- 2 new SDK messages added
- FAQ added

#### December 13, 2003
- **FirstPass analyzer** added (modify/add/delete data addresses)

#### December 12, 2003
- **Chip8 CPU** added to CPU category (binary file disassembly)

#### December 2, 2003
- CodePatcher bug fixed (prefix size was not added)
- Custom data sections (segments) support

#### November 11, 2003
- **Save/Load Project** added (7 project files)
- **Plugin SDK** released &mdash; code your own plugins for PVDasm
  - Ships with: **CLD.DLL** (Command Line Disassembler plugin)

#### November 1, 2003
- Dockable Debug Window
- Search within disassembled code
- **XReferences** support (`Ctrl+Space`)
- **HexEditor** (RadASM add-in by KetilO, ported to VC++)
- String & Import References dialogs with search
- **CodePatcher** &mdash; inline patcher with assembly preview
- GUI fixes and SEH frame additions

#### October 12, 2003
- **Branch Tracing/Back Tracing** (`Left`/`Right` arrow keys, or double-click `JXX`)
- Toolbar upgraded

#### October 1, 2003
- **100% disassembly speed** improvement (x8 optimization!)
- Color schemes: SoftICE, IDA, OllyDbg, W32Asm, Custom
- Goto Address/EntryPoint/Code Start options
- WinXP Theme (Manifest) support
- String References with search dialog

#### August 27, 2003
- Import resolving & Imports dialog with search
- Virtual ListView for large data
- Auto memory allocation based on disassembled code
- Double-click to add/edit comments

#### August 15, 2003
- **Disasm Engine complete!** Full 0F set: MMX, 3DNow!, SSE, SSE2
- Disasm from EntryPoint option

#### August 10, 2003
- Disassembler menu options added
- Progress bar / percent indicator
- Force disasm bytes from EP (user-defined: 0&ndash;50 bytes)

#### August 9, 2003
- Partial 0F instruction set support (JXX & 1-byte opcodes)

#### August 8, 2003
- Force disassembly before EP option
- Auto jump to EP / code start / address
- Restart disassembly option

#### August 7, 2003
- Process Viewer/Dumper supports 9x/2K/XP

#### May 18, 2003
- Disassembler implemented

#### February 10, 2003
- Added Exports viewer

#### February 5, 2003
- Added Import viewer in PE Editor / Directory viewer

#### February 3, 2003
- PE Rebuilder added (alignment, headers, sections)
- Partial Process Dumper (address & size selectable)

<p align="center">
  <sub>Made with dedication by <strong>Shany Golan</strong> &mdash; 2003&ndash;2026</sub>
</p>
