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

 File: VBPCode.cpp
 Visual Basic 5 / Visual Basic 6 PCODE Disassembly Engine.

 Decodes PCODE bytecode from VB5 and VB6 compiled executables.
 PCODE is produced when a VB project is compiled in "P-Code" mode.
 The VB runtime (MSVBVM50.DLL / MSVBVM60.DLL) interprets PCODE at run-time.

 Opcode table: VBOpcodeTable.h (auto-generated from vb_opcodes.json).
 See also: pcode_information/ for VB PE structure and opcode documentation.
*/

#include "Disasm.h"
#include "Chip8.h"
#include "VBPCode.h"
#include "CDisasmData.h"
#include "VBOpcodeTable.h"
#include <vector>
#include <map>
#include <cctype>

using namespace std;

//////////////////////////////////////////////////////////////////////////
//                        TYPE DEFINES                                  //
//////////////////////////////////////////////////////////////////////////

typedef vector<CDisasmData>         DisasmDataArray;
typedef multimap<const int, int>    XRef, MapTree;
typedef XRef::iterator              ItrXref;
typedef vector<CODE_BRANCH>         CodeBranch;
typedef pair<const int const, int>  XrefAdd, MapTreeAdd;

//////////////////////////////////////////////////////////////////////////
//                            DEFINES                                   //
//////////////////////////////////////////////////////////////////////////

#define VBAddNew(key, data) XrefData.insert(XrefAdd(key, data));

//////////////////////////////////////////////////////////////////////////
//                      EXTERNAL VARIABLES                              //
//////////////////////////////////////////////////////////////////////////

extern DWORD_PTR        hFileSize;
extern char*            OrignalData;
extern DisasmDataArray  DisasmDataLines;
extern XRef             XrefData;
extern DISASM_OPTIONS   disop;
extern HWND             hWndTB, Main_hWnd;
extern bool             DisassemblerReady;
extern HANDLE           hDisasmThread;
extern CodeBranch       DisasmCodeFlow;

//////////////////////////////////////////////////////////////////////////
//              VA -> FILE OFFSET CONVERSION UTILITY                    //
//////////////////////////////////////////////////////////////////////////

DWORD_PTR VBVAToFileOffset(const BYTE* pFileData, DWORD_PTR fileSize, DWORD va)
{
    if (!pFileData || fileSize < sizeof(IMAGE_DOS_HEADER))
        return (DWORD_PTR)-1;

    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)pFileData;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return (DWORD_PTR)-1;

    if ((DWORD_PTR)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > fileSize)
        return (DWORD_PTR)-1;

    const IMAGE_NT_HEADERS* nt =
        (const IMAGE_NT_HEADERS*)(pFileData + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return (DWORD_PTR)-1;

    DWORD rva = va - nt->OptionalHeader.ImageBase;
    WORD  numSec = nt->FileHeader.NumberOfSections;
    const IMAGE_SECTION_HEADER* sec =
        (const IMAGE_SECTION_HEADER*)((const BYTE*)&nt->OptionalHeader +
                                       nt->FileHeader.SizeOfOptionalHeader);

    for (WORD i = 0; i < numSec; i++) {
        DWORD vStart = sec[i].VirtualAddress;
        DWORD vSize  = (sec[i].Misc.VirtualSize > 0) ? sec[i].Misc.VirtualSize
                                                      : sec[i].SizeOfRawData;
        if (rva >= vStart && rva < vStart + vSize) {
            DWORD fileOff = sec[i].PointerToRawData + (rva - vStart);
            return (fileOff < fileSize) ? (DWORD_PTR)fileOff : (DWORD_PTR)-1;
        }
    }

    // Header area: RVA < SizeOfHeaders means file offset == RVA
    // (import tables rebuilt into header slack by PE rebuild)
    if (rva < nt->OptionalHeader.SizeOfHeaders && rva < fileSize)
        return (DWORD_PTR)rva;

    return (DWORD_PTR)-1;
}

//////////////////////////////////////////////////////////////////////////
//              VB RUNTIME DETECTION (IMPORT TABLE SCAN)                //
//////////////////////////////////////////////////////////////////////////

BOOL DetectVBRuntime(const BYTE* pFileData, DWORD_PTR fileSize, int* pVersion)
{
    if (!pFileData || fileSize < sizeof(IMAGE_DOS_HEADER) || !pVersion)
        return FALSE;

    *pVersion = 0;

    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)pFileData;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    if ((DWORD_PTR)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > fileSize)
        return FALSE;

    const IMAGE_NT_HEADERS* nt =
        (const IMAGE_NT_HEADERS*)(pFileData + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    IMAGE_DATA_DIRECTORY importDir =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.VirtualAddress == 0 || importDir.Size == 0)
        return FALSE;

    DWORD imgBase = nt->OptionalHeader.ImageBase;

    // Import directory fields are RVAs, add ImageBase to convert to VA
    DWORD_PTR impOff = VBVAToFileOffset(pFileData, fileSize,
                                         importDir.VirtualAddress + imgBase);
    if (impOff == (DWORD_PTR)-1)
        return FALSE;

    const IMAGE_IMPORT_DESCRIPTOR* imp =
        (const IMAGE_IMPORT_DESCRIPTOR*)(pFileData + impOff);

    while (impOff + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= fileSize && imp->Name != 0)
    {
        DWORD_PTR nameOff = VBVAToFileOffset(pFileData, fileSize, imp->Name + imgBase);
        if (nameOff != (DWORD_PTR)-1 && nameOff < fileSize) {
            const char* dll = (const char*)(pFileData + nameOff);
            char upper[64] = {0};
            for (int k = 0; k < 63 && dll[k]; k++)
                upper[k] = (char)toupper((unsigned char)dll[k]);

            if (strcmp(upper, "MSVBVM50.DLL") == 0) { *pVersion = 5; return TRUE; }
            if (strcmp(upper, "MSVBVM60.DLL") == 0) { *pVersion = 6; return TRUE; }
        }
        imp++;
        impOff += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//              FIRST EXECUTABLE SECTION FINDER (FALLBACK)             //
//////////////////////////////////////////////////////////////////////////

static BOOL FindCodeSection(const BYTE* pFileData, DWORD_PTR fileSize,
                              DWORD_PTR* pSecFileOff, DWORD_PTR* pSecSize,
                              DWORD_PTR* pSecVA)
{
    if (!pFileData || fileSize < sizeof(IMAGE_DOS_HEADER))
        return FALSE;

    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)pFileData;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    if ((DWORD_PTR)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > fileSize)
        return FALSE;

    const IMAGE_NT_HEADERS* nt =
        (const IMAGE_NT_HEADERS*)(pFileData + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    WORD numSec = nt->FileHeader.NumberOfSections;
    const IMAGE_SECTION_HEADER* sec =
        (const IMAGE_SECTION_HEADER*)((const BYTE*)&nt->OptionalHeader +
                                       nt->FileHeader.SizeOfOptionalHeader);

    for (WORD i = 0; i < numSec; i++) {
        DWORD flags = sec[i].Characteristics;
        if ((flags & IMAGE_SCN_CNT_CODE) || (flags & IMAGE_SCN_MEM_EXECUTE)) {
            if (pSecFileOff) *pSecFileOff = sec[i].PointerToRawData;
            if (pSecSize)    *pSecSize    = sec[i].SizeOfRawData;
            if (pSecVA)
                *pSecVA = (DWORD_PTR)sec[i].VirtualAddress +
                          nt->OptionalHeader.ImageBase;
            return TRUE;
        }
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//          VB PCODE REGION FINDER (VBHeader -> ProjectData chain)     //
//////////////////////////////////////////////////////////////////////////

BOOL FindVBPCodeRegion(const BYTE* pFileData, DWORD_PTR fileSize,
                        DWORD_PTR imageBase,
                        DWORD_PTR* pCodeOffset, DWORD_PTR* pCodeSize)
{
    if (!pFileData || fileSize < sizeof(IMAGE_DOS_HEADER))
        return FALSE;

    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)pFileData;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    if ((DWORD_PTR)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > fileSize)
        return FALSE;

    const IMAGE_NT_HEADERS* nt =
        (const IMAGE_NT_HEADERS*)(pFileData + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    // Locate entry point in file
    DWORD epRVA    = nt->OptionalHeader.AddressOfEntryPoint;
    DWORD imgBase  = nt->OptionalHeader.ImageBase;
    DWORD_PTR epFO = VBVAToFileOffset(pFileData, fileSize, imgBase + epRVA);

    if (epFO == (DWORD_PTR)-1 || epFO + 10 >= fileSize)
        return FALSE;

    // Look for the VB entry stub: 68 <imm32>  E8 <rel32>
    // (PUSH VBHeaderVA) (CALL ThunRTMain)
    DWORD vbHeaderVA = 0;
    for (DWORD_PTR k = 0; k < 32 && epFO + k + 6 < fileSize; k++) {
        if (pFileData[epFO + k] == 0x68 &&
            pFileData[epFO + k + 5] == 0xE8) {
            vbHeaderVA = *(const DWORD*)(pFileData + epFO + k + 1);
            break;
        }
    }
    if (vbHeaderVA == 0)
        return FALSE;

    // Map VBHeader VA -> file offset
    DWORD_PTR vbHdrFO = VBVAToFileOffset(pFileData, fileSize, vbHeaderVA);
    if (vbHdrFO == (DWORD_PTR)-1 ||
        vbHdrFO + sizeof(VB_HEADER) > fileSize)
        return FALSE;

    // Validate magic
    const VB_HEADER* hdr = (const VB_HEADER*)(pFileData + vbHdrFO);
    if (hdr->dwSignature != VB_HEADER_MAGIC)
        return FALSE;

    if (hdr->lpProjectData == 0)
        return FALSE;

    // Follow lpProjectData -> VB_PROJECT_DATA
    DWORD_PTR projFO = VBVAToFileOffset(pFileData, fileSize, hdr->lpProjectData);
    if (projFO == (DWORD_PTR)-1 ||
        projFO + sizeof(VB_PROJECT_DATA) > fileSize)
        return FALSE;

    const VB_PROJECT_DATA* proj = (const VB_PROJECT_DATA*)(pFileData + projFO);

    if (proj->lpCodeStart == 0 || proj->lpCodeEnd == 0 ||
        proj->lpCodeEnd <= proj->lpCodeStart)
        return FALSE;

    DWORD_PTR codeStartFO =
        VBVAToFileOffset(pFileData, fileSize, proj->lpCodeStart);
    if (codeStartFO == (DWORD_PTR)-1)
        return FALSE;

    DWORD_PTR codeBytes = proj->lpCodeEnd - proj->lpCodeStart;
    if (codeStartFO + codeBytes > fileSize)
        codeBytes = fileSize - codeStartFO;

    *pCodeOffset = codeStartFO;
    *pCodeSize   = codeBytes;
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//                     OPERAND FORMATTING HELPER                       //
//////////////////////////////////////////////////////////////////////////

// Append raw argument bytes (as space-separated hex pairs) to opcodeHexStr.
static void AppendArgBytesHex(char* buf, const BYTE* pData,
                               DWORD_PTR from, int count, DWORD_PTR fileSize)
{
    char tmp[8];
    for (int i = 0; i < count && from + i < fileSize; i++) {
        wsprintf(tmp, " %02X", pData[from + i]);
        if (lstrlen(buf) + 3 < 24)
            lstrcat(buf, tmp);
    }
}

// Build the Assembly display string from the opcode entry and inline argument bytes.
static void BuildVBAssembly(char* outAsm, int outLen,
                              const VB_OPCODE_ENTRY* entry,
                              const BYTE* pData, DWORD_PTR argOffset,
                              DWORD_PTR fileSize)
{
    lstrcpyn(outAsm, entry->name ? entry->name : "???", outLen);

    if (!entry->argStr || argOffset >= fileSize)
        return;

    const char* a   = entry->argStr;
    char tok[72]    = "";
    DWORD_PTR pos   = argOffset;

    // Walk the ArgStr token by token
    while (*a && pos <= fileSize) {
        tok[0] = '\0';

        if (a[0] == '>' && a[1] == 'p') {
            // Signed WORD branch offset
            if (pos + 2 <= fileSize) {
                SHORT v = *(const SHORT*)(pData + pos);
                wsprintf(tok, " %+d", (int)v);
                pos += 2;
            }
            a += 2;
        } else if (a[0] == 'u' && a[1] == '%') {
            // WORD immediate
            if (pos + 2 <= fileSize) {
                wsprintf(tok, " 0x%04X", (unsigned)*(const WORD*)(pData + pos));
                pos += 2;
            }
            a += 2;
        } else if (a[0] == 'u' && a[1] == '&') {
            // DWORD immediate
            if (pos + 4 <= fileSize) {
                wsprintf(tok, " 0x%08X", *(const DWORD*)(pData + pos));
                pos += 4;
            }
            a += 2;
        } else if (a[0] == 'u' && a[1] == 'c') {
            // BYTE
            if (pos + 1 <= fileSize) {
                wsprintf(tok, " 0x%02X", (unsigned)pData[pos]);
                pos += 1;
            }
            a += 2;
        } else if (a[0] == 'u' && a[1] == 'a' && a[2] == 'z') {
            // DWORD string pointer
            if (pos + 4 <= fileSize) {
                wsprintf(tok, " 0x%08X", *(const DWORD*)(pData + pos));
                pos += 4;
            }
            a += 3;
        } else if (a[0] == 'u' && a[1] == 'l') {
            // DWORD pointer
            if (pos + 4 <= fileSize) {
                wsprintf(tok, " 0x%08X", *(const DWORD*)(pData + pos));
                pos += 4;
            }
            a += 2;
        } else if (a[0] == 'u' && a[1] == 'm') {
            // Variable/member reference (DWORD) — um%, um&, um!, um#, um$, ums, umv
            if (pos + 4 <= fileSize) {
                wsprintf(tok, " [0x%08X]", *(const DWORD*)(pData + pos));
                pos += 4;
            }
            a += 3;  // skip "um" + type character
        } else if (a[0] == 'l' && (a[1] == '\0' || a[1] == '>' || a[1] == 'u')) {
            // DWORD local offset; also handles "lu%" / "ln" by consuming 'l' then looping
            if (pos + 4 <= fileSize) {
                wsprintf(tok, " [0x%08X]", *(const DWORD*)(pData + pos));
                pos += 4;
            }
            a += 1;
        } else if (a[0] == 'v' && a[1] == 'w') {
            // Virtual call: DWORD vtable + WORD index
            if (pos + 6 <= fileSize) {
                DWORD vt = *(const DWORD*)(pData + pos);
                WORD  vi = *(const WORD*) (pData + pos + 4);
                wsprintf(tok, " vtbl:0x%08X,idx:%u", vt, (unsigned)vi);
                pos += 6;
            }
            a += 2;
        } else if (a[0] == 'h' && (a[1] == '%' || a[1] == '&')) {
            // WORD or DWORD handle
            BOOL isWord = (a[1] == '%');
            int  bytes  = isWord ? 2 : 4;
            if (pos + bytes <= fileSize) {
                if (isWord)
                    wsprintf(tok, " h:0x%04X", (unsigned)*(const WORD*)(pData + pos));
                else
                    wsprintf(tok, " h:0x%08X", *(const DWORD*)(pData + pos));
                pos += bytes;
            }
            a += 2;
        } else if (a[0] == 'a' && a[1] == '2') {
            // 2-byte address reference
            if (pos + 2 <= fileSize) {
                wsprintf(tok, " 0x%04X", (unsigned)*(const WORD*)(pData + pos));
                pos += 2;
            }
            a += 2;
        } else if (a[0] == 'w') {
            // WORD argument
            if (pos + 2 <= fileSize) {
                wsprintf(tok, " 0x%04X", (unsigned)*(const WORD*)(pData + pos));
                pos += 2;
            }
            a += 1;
        } else if (a[0] == '}') {
            // End-of-proc marker — no bytes consumed
            a++;
        } else {
            // Unrecognized token character — skip it
            a++;
        }

        if (tok[0] && lstrlen(outAsm) + lstrlen(tok) < outLen - 1)
            lstrcat(outAsm, tok);
    }
}

//////////////////////////////////////////////////////////////////////////
//             CODE FLOW FLAG DETECTION (name-based heuristics)        //
//////////////////////////////////////////////////////////////////////////

static void ApplyVBCodeFlow(CODE_FLOW* cf, const VB_OPCODE_ENTRY* entry)
{
    if (!entry || !entry->name) return;

    const char* nm  = entry->name;
    const char* arg = entry->argStr ? entry->argStr : "";

    // Branches: ArgStr contains ">p" (relative signed offset)
    if (strstr(arg, ">p"))
        cf->Jump = TRUE;

    // Named branch patterns
    if (strncmp(nm, "Branch",  6) == 0) cf->Jump = TRUE;
    if (strncmp(nm, "For",     3) == 0) cf->Jump = TRUE;
    if (strncmp(nm, "Next",    4) == 0) cf->Jump = TRUE;
    if (strncmp(nm, "OnError", 7) == 0) cf->Jump = TRUE;

    // Call patterns
    if (strstr(nm, "VCall") || strstr(nm, "ImpAd") ||
        strncmp(nm, "Call", 4) == 0)
        cf->Call = TRUE;

    // Return patterns
    if (strncmp(nm, "ExitProc", 8) == 0 || strncmp(nm, "Return", 6) == 0)
        cf->Ret = TRUE;
}

//////////////////////////////////////////////////////////////////////////
//                    CORE INSTRUCTION DECODER                         //
//////////////////////////////////////////////////////////////////////////

void DisasmVBPCodeInstr(const BYTE* pData, DWORD_PTR* pOffset, DWORD_PTR fileSize,
                         DISASSEMBLY* Disasm, DWORD_PTR* pBranchAddr)
{
    *pBranchAddr = 0;

    if (*pOffset >= fileSize) {
        lstrcpy(Disasm->Assembly, "???");
        Disasm->OpcodeSize = 1;
        lstrcpy(Disasm->Opcode, "??");
        return;
    }

    BYTE firstByte    = pData[*pOffset];
    DWORD_PTR hdrBytes = 1;  // opcode header bytes (1 = base, 2 = extended)
    const VB_OPCODE_ENTRY* entry = NULL;
    char rawHex[25] = "";

    if (firstByte >= 0xFB) {
        // Two-byte extended opcode
        if (*pOffset + 1 >= fileSize) {
            wsprintf(rawHex, "%02X ??", firstByte);
            lstrcpy(Disasm->Assembly, "???");
            Disasm->OpcodeSize = 1;
            lstrcpy(Disasm->Opcode, rawHex);
            *pOffset += 1;
            return;
        }
        BYTE secByte = pData[*pOffset + 1];
        switch (firstByte) {
            case 0xFB: entry = &VBLead0Opcodes[secByte]; break;
            case 0xFC: entry = &VBLead1Opcodes[secByte]; break;
            case 0xFD: entry = &VBLead2Opcodes[secByte]; break;
            case 0xFE: entry = &VBLead3Opcodes[secByte]; break;
            case 0xFF: entry = &VBLead4Opcodes[secByte]; break;
            default:   break;
        }
        wsprintf(rawHex, "%02X %02X", firstByte, secByte);
        hdrBytes = 2;
    } else {
        entry = &VBBaseOpcodes[firstByte];
        wsprintf(rawHex, "%02X", firstByte);
        hdrBytes = 1;
    }

    // Determine total instruction length
    DWORD_PTR totalLen = hdrBytes;
    if (entry && entry->length > 0)
        totalLen = (DWORD_PTR)entry->length;

    // Clamp to available data
    if (*pOffset + totalLen > fileSize)
        totalLen = fileSize - *pOffset;
    if (totalLen < hdrBytes)
        totalLen = hdrBytes;

    // Append argument bytes to raw hex display string
    DWORD_PTR argBytes = totalLen - hdrBytes;
    AppendArgBytesHex(rawHex, pData, *pOffset + hdrBytes, (int)argBytes, fileSize);

    // Build the human-readable Assembly string
    if (entry && entry->name && entry->name[0] != '?') {
        BuildVBAssembly(Disasm->Assembly, sizeof(Disasm->Assembly),
                        entry, pData, *pOffset + hdrBytes, fileSize);
    } else {
        wsprintf(Disasm->Assembly, "db %s", rawHex);
    }

    lstrcpy(Disasm->Opcode, rawHex);
    Disasm->OpcodeSize = totalLen;
    Disasm->PrefixSize = 0;

    // Set CODE_FLOW flags
    if (entry) {
        ApplyVBCodeFlow(&Disasm->CodeFlow, entry);

        // Compute branch target for ">p" style branches
        if (Disasm->CodeFlow.Jump && strstr(entry->argStr ? entry->argStr : "", ">p")) {
            DWORD_PTR offPos = *pOffset + totalLen - 2;  // last 2 bytes = offset
            if (offPos + 2 <= fileSize) {
                SHORT relOff = *(const SHORT*)(pData + offPos);
                // Branch target VA: Disasm->Address points to current instruction VA
                // next instruction VA = Disasm->Address + totalLen
                *pBranchAddr = (DWORD_PTR)((LONG_PTR)(Disasm->Address + totalLen)
                                            + (LONG_PTR)relOff);
            }
        }
    }

    // Advance the file offset past this instruction
    *pOffset += totalLen;
}

//////////////////////////////////////////////////////////////////////////
//                    MAIN PCODE THREAD FUNCTION                       //
//////////////////////////////////////////////////////////////////////////

void VBPCode()
{
    DISASSEMBLY Disasm;
    CODE_BRANCH CBranch;
    HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
    HMENU hMenu  = GetMenu(Main_hWnd);
    HWND DbgWin  = GetDlgItem(Main_hWnd, IDC_LIST);

    DWORD_PTR ListIndex = 0;
    DWORD_PTR percent   = 0;
    DWORD_PTR step      = (DWORD_PTR)-1;

    const BYTE* pData   = (const BYTE*)OrignalData;
    DWORD_PTR   fileSize = hFileSize;

    // ------------------------------------------------------------------
    // Initialise progress bar and disable navigation toolbar buttons
    // ------------------------------------------------------------------
    SendDlgItemMessage(Main_hWnd, IDC_DISASM_PROGRESS, PBM_SETRANGE32,
                       (WPARAM)0, (LPARAM)fileSize);
    ShowWindow(GetDlgItem(Main_hWnd, IDC_DISASM_PROGRESS), SW_SHOW);
    EnableMenuItem(hMenu, IDC_START_DISASM, MF_GRAYED);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,  (LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,  (LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_JUMP, (LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_CALL, (LPARAM)FALSE);

    OutDebug(Main_hWnd, "+----------------------------------------------------+");
    OutDebug(Main_hWnd, "|         VB P-Code Disassembly Analysis              |");
    OutDebug(Main_hWnd, "+----------------------------------------------------+");
    Sleep(55);
    SelectLastItem(DbgWin);

    // ------------------------------------------------------------------
    // Detect VB version and obtain image base
    // ------------------------------------------------------------------
    int vbVersion = disop.VBVersion;
    if (vbVersion == 0)
        DetectVBRuntime(pData, fileSize, &vbVersion);

    char vbMsg[128];
    wsprintf(vbMsg, "Detected: Visual Basic %d P-Code binary.", vbVersion);
    OutDebug(Main_hWnd, vbMsg);
    Sleep(55);
    SelectLastItem(DbgWin);

    DWORD_PTR imageBase = 0x00400000;
    {
        const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)pData;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE &&
            (DWORD_PTR)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) <= fileSize) {
            const IMAGE_NT_HEADERS* nt =
                (const IMAGE_NT_HEADERS*)(pData + dos->e_lfanew);
            if (nt->Signature == IMAGE_NT_SIGNATURE)
                imageBase = nt->OptionalHeader.ImageBase;
        }
    }

    // ------------------------------------------------------------------
    // Locate the PCODE region (VBHeader chain, or fallback to .text)
    // ------------------------------------------------------------------
    DWORD_PTR codeFileOff = 0;
    DWORD_PTR codeSize    = 0;
    DWORD_PTR codeBaseVA  = imageBase;

    BOOL parsedOK = FindVBPCodeRegion(pData, fileSize, imageBase,
                                       &codeFileOff, &codeSize);
    if (parsedOK) {
        // Derive the VA corresponding to codeFileOff
        DWORD_PTR secFO = 0, secSz = 0, secVA = 0;
        if (FindCodeSection(pData, fileSize, &secFO, &secSz, &secVA) &&
            codeFileOff >= secFO) {
            codeBaseVA = secVA + (codeFileOff - secFO);
        }
        OutDebug(Main_hWnd, "VBHeader parsed: P-Code region located via ProjectData.");
    } else {
        DWORD_PTR secSz = 0;
        if (FindCodeSection(pData, fileSize, &codeFileOff, &secSz, &codeBaseVA)) {
            codeSize = secSz;
            OutDebug(Main_hWnd,
                     "VBHeader not found; decoding first executable section as P-Code.");
        } else {
            codeFileOff = 0;
            codeSize    = fileSize;
            OutDebug(Main_hWnd, "No code section found; decoding entire file.");
        }
    }

    char regionMsg[128];
    wsprintf(regionMsg,
             "P-Code: file offset 0x%08X, size 0x%08X bytes, base VA 0x%08X.",
             (DWORD)codeFileOff, (DWORD)codeSize, (DWORD)codeBaseVA);
    OutDebug(Main_hWnd, regionMsg);
    Sleep(55);
    SelectLastItem(DbgWin);

    // ------------------------------------------------------------------
    // Main decode loop
    // ------------------------------------------------------------------
    DWORD_PTR offset  = codeFileOff;
    DWORD_PTR codeEnd = codeFileOff + codeSize;
    if (codeEnd > fileSize) codeEnd = fileSize;

    Disasm.Address = codeBaseVA;

    while (offset < codeEnd) {
        percent = (codeSize > 0)
            ? (DWORD_PTR)(((float)(offset - codeFileOff) / (float)codeSize) * 100.0f)
            : 0;

        DWORD_PTR branchAddr = 0;
        FlushDecoded(&Disasm);
        // Preserve current VA before DisasmVBPCodeInstr advances offset
        DWORD_PTR currentVA = Disasm.Address;

        DisasmVBPCodeInstr(pData, &offset, fileSize, &Disasm, &branchAddr);
        // (offset has been advanced by OpcodeSize inside DisasmVBPCodeInstr)

        // Track jumps and calls for XRef
        if ((Disasm.CodeFlow.Jump || Disasm.CodeFlow.Call) && branchAddr != 0) {
            if (branchAddr >= codeBaseVA && branchAddr < codeBaseVA + codeSize) {
                VBAddNew((DWORD)branchAddr, (DWORD)ListIndex);

                CBranch.Current_Index      = ListIndex;
                CBranch.Branch_Destination = branchAddr;
                CBranch.BranchFlow.Call    = Disasm.CodeFlow.Call;
                CBranch.BranchFlow.Jump    = Disasm.CodeFlow.Jump;
                DisasmCodeFlow.insert(DisasmCodeFlow.end(), CBranch);
            }
        }

        AddNewDecode(Disasm, (DWORD)ListIndex);
        ListIndex++;

        // Advance the display address for the next instruction
        Disasm.Address = codeBaseVA + (offset - codeFileOff);

        if (percent > step) {
            step = percent;
            PostMessage(GetDlgItem(Main_hWnd, IDC_DISASM_PROGRESS),
                        PBM_SETPOS, (WPARAM)(offset - codeFileOff), 0);
            PostMessage(Main_hWnd, WM_USER + MY_MSG, 0, (LPARAM)currentVA);
        }
    }

    // ------------------------------------------------------------------
    // Wrap up
    // ------------------------------------------------------------------
    DisassemblerReady = TRUE;
    RefreshCodeMapBar();
    ShowWindow(GetDlgItem(Main_hWnd, IDC_DISASM_PROGRESS), SW_HIDE);
    SendDlgItemMessage(Main_hWnd, IDC_DISASM_PROGRESS, PBM_SETPOS, 0, 0);
    ListView_SetItemCountEx(hDisasm, (int)ListIndex, NULL);

    EnableMenuItem(hMenu, IDC_STOP_DISASM,  MF_GRAYED);
    EnableMenuItem(hMenu, IDC_PAUSE_DISASM, MF_GRAYED);
    EnableMenuItem(hMenu, IDC_START_DISASM, MF_ENABLED);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SEARCH, (LPARAM)TRUE);
    EnableMenuItem(hMenu, IDM_FIND, MF_ENABLED);

    char summary[128];
    wsprintf(summary,
             "VB P-Code Disassembly Complete. %u instructions decoded.",
             (DWORD)ListIndex);
    SetDlgItemText(Main_hWnd, IDC_MESSAGE1, summary);
    SetDlgItemText(Main_hWnd, IDC_MESSAGE2, "");
    SetDlgItemText(Main_hWnd, IDC_MESSAGE3, "");

    OutDebug(Main_hWnd, "");
    OutDebug(Main_hWnd, summary);
    OutDebug(Main_hWnd, "------------------------------------------------------");
    Sleep(55);
    SelectLastItem(DbgWin);

    ExitThread(0);
}
