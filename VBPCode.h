/*
     8888888b.                  888     888 d8b
     888   Y88b                 888     888 Y8P
     888    888                 888     888
     888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888
     8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888
     888        888     888  888  Y88o88P   888 88888888 888  888  888
     888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P
     888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"


             PE Editor & Dissasembler & File Identifier
             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 File: VBPCode.h
 Visual Basic 5 / Visual Basic 6 PCODE Disassembly Engine.

 PCODE is the pseudo-code bytecode produced when a VB5/VB6 project is
 compiled in "P-Code" mode (as opposed to Native Code). The VB runtime
 (MSVBVM50.DLL or MSVBVM60.DLL) interprets the PCODE at run-time.

 Opcode tables are sourced from vb_opcodes.json and pre-generated into
 VBOpcodeTable.h by tools/gen_vb_opcodes.py.

 Detection: VB5/VB6 PE files import ThunRTMain from MSVBVM50.DLL / MSVBVM60.DLL.
 PCODE location: Parsed from the VB runtime header structure (VBHeader) which
 immediately follows the entry-point stub.
*/

#ifndef _VBPCODE_H_
#define _VBPCODE_H_

#include <windows.h>
#include "Disasm.h"

//////////////////////////////////////////////////////////////////////////
//                  VB RUNTIME DETECTION CONSTANTS                      //
//////////////////////////////////////////////////////////////////////////

#define VB5_RUNTIME_DLL     "MSVBVM50.DLL"
#define VB6_RUNTIME_DLL     "MSVBVM60.DLL"
#define VB5_RUNTIME_DLL_LC  "msvbvm50.dll"
#define VB6_RUNTIME_DLL_LC  "msvbvm60.dll"

// "VB5!" magic signature stored in the VBHeader structure
// Bytes: 56 42 35 21  ('V','B','5','!')
#define VB_HEADER_MAGIC     0x21354256UL

//////////////////////////////////////////////////////////////////////////
//               VB INTERNAL PE STRUCTURE DEFINITIONS                   //
//////////////////////////////////////////////////////////////////////////
// These structures are embedded in every VB5/VB6 PCODE-compiled PE.
// Reference: vb-decompiler.org forum archives (pcode_information/).

#pragma pack(push, 1)

// VBHeader — pointed to by the PUSH argument at the PE entry point.
// Entry point pattern:  PUSH offset VBHeader / CALL ThunRTMain
typedef struct _VB_HEADER {
    DWORD  dwSignature;         // "VB5!" (0x21354256)
    WORD   wRuntimeBuild;       // Runtime build number
    CHAR   szLangDll[14];       // Primary language DLL name
    CHAR   szSecLangDll[14];    // Secondary language DLL name
    WORD   wRuntimeRevision;    // Runtime revision
    DWORD  dwLCID;              // Primary locale ID
    DWORD  dwSecLCID;           // Secondary locale ID
    DWORD  lpSubMain;           // VA of project Sub Main (0 if none)
    DWORD  lpProjectData;       // VA of VB_PROJECT_DATA structure
    DWORD  fMDLIntObjs;         // Internal object flags
    DWORD  fFeatureData;        // Feature flags
    DWORD  dwReserved1;
    DWORD  dwReserved2;
    DWORD  dwReserved3;
    DWORD  dwReserved4;
    DWORD  dwReserved5;
    DWORD  dwReserved6;
} VB_HEADER;

// VB_PROJECT_DATA — contains pointers to all project-level data.
typedef struct _VB_PROJECT_DATA {
    DWORD  dwVersion;           // Project data version
    DWORD  lpObjectTable;       // VA of VB_OBJECT_TABLE
    DWORD  dwNull;
    DWORD  lpCodeStart;         // VA of first byte of PCODE
    DWORD  lpCodeEnd;           // VA of last byte of PCODE + 1
    DWORD  dwDataSize;
    DWORD  lpThreadSpace;
    DWORD  lpVbaSeh;
    DWORD  lpNativeCode;
    WCHAR  szPathInformation[528];
    DWORD  lpExternalTable;
    DWORD  dwExternalCount;
} VB_PROJECT_DATA;

// VB_OBJECT_TABLE — project-level object/module registry.
typedef struct _VB_OBJECT_TABLE {
    DWORD  lpHeapLink;
    DWORD  lpExecProj;
    DWORD  lpProjectInfo2;
    DWORD  dwReserved;
    DWORD  dwNull;
    DWORD  lpProjectObject;
    GUID   uuidObject;
    WORD   wTotalObjects;       // Total number of objects/modules in project
    WORD   wCompiledObjects;
    WORD   wObjectsInUse;
    WORD   wReserved1;
    DWORD  lpObjectArray;       // VA of array of per-object VB_OBJECT_INFO pointers
    DWORD  dwFlags;
    DWORD  lpHeapLink2;
    DWORD  lpStartOfCode;       // VA of the start of the PCODE block
} VB_OBJECT_TABLE;

#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////
//                    FUNCTION DECLARATIONS                             //
//////////////////////////////////////////////////////////////////////////

// Main PCODE disassembly thread (call via CreateThread, mirrors Chip8())
void VBPCode();

// Core per-instruction decoder
//   pData      : pointer to start of file data buffer
//   pOffset    : current byte offset into pData; advanced by instruction length on return
//   fileSize   : total file size (bounds checking)
//   Disasm     : output DISASSEMBLY struct to populate
//   pBranchAddr: if the instruction is a branch/call, receives the target address; else 0
void DisasmVBPCodeInstr(const BYTE* pData, DWORD_PTR* pOffset, DWORD_PTR fileSize,
                         DISASSEMBLY* Disasm, DWORD_PTR* pBranchAddr);

// Scan the PE import table for MSVBVM50.DLL or MSVBVM60.DLL.
// Returns TRUE if found; sets *pVersion to 5 or 6.
BOOL DetectVBRuntime(const BYTE* pFileData, DWORD_PTR fileSize, int* pVersion);

// Walk the VBHeader → ProjectData → ObjectTable chain to find the PCODE region.
// imageBase is the preferred load address from the PE optional header.
// On success returns TRUE and sets *pCodeOffset (file offset) and *pCodeSize.
// On failure returns FALSE; caller should fall back to scanning the code section.
BOOL FindVBPCodeRegion(const BYTE* pFileData, DWORD_PTR fileSize, DWORD_PTR imageBase,
                        DWORD_PTR* pCodeOffset, DWORD_PTR* pCodeSize);

// Utility: convert a Virtual Address (VA) to a file offset using the PE section table.
// Returns (DWORD_PTR)-1 if the VA is not mapped to any section.
DWORD_PTR VBVAToFileOffset(const BYTE* pFileData, DWORD_PTR fileSize, DWORD va);

#endif // _VBPCODE_H_
