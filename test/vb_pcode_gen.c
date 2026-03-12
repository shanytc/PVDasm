/*
 * vb_pcode_gen.c - Generates a minimal VB6 P-Code PE for PVDasm
 *
 * Creates vb_pcode.exe: a minimal 32-bit PE file containing:
 *   - Valid MZ/PE headers
 *   - Entry point stub: PUSH VBHeaderVA / CALL +0 / RET
 *   - VB_HEADER with "VB5!" signature (0x21354256)
 *   - VB_PROJECT_DATA with lpCodeStart / lpCodeEnd
 *   - P-Code bytecode containing a variety of opcodes
 *
 * PVDasm will detect the VBHeader chain and decode the P-Code region.
 *
 * Compile: cl /nologo /Fe:vb_pcode_gen.exe vb_pcode_gen.c /link /SUBSYSTEM:CONSOLE
 * Run:     vb_pcode_gen.exe   (produces vb_pcode.exe)
 */
#include <stdio.h>
#include <string.h>

/* PE layout constants */
#define IMAGE_BASE      0x00400000
#define SECT_ALIGN      0x1000
#define FILE_ALIGN      0x0200
#define TEXT_RVA        0x1000
#define TEXT_FILE_OFF   0x0200

/* Structures within .text section (RVAs) */
#define EP_RVA          0x1000      /* entry point */
#define VBHDR_RVA       0x1020      /* VB_HEADER */
#define PROJDATA_RVA    0x1080      /* VB_PROJECT_DATA (first 0x14 bytes) */
#define IMPORT_DIR_RVA  0x10A0      /* Import directory table */
#define IMPORT_NAME_RVA 0x10D0      /* "MSVBVM60.DLL" string */
#define PCODE_RVA       0x1200      /* P-Code start */
#define PCODE_END_RVA   0x1400      /* P-Code end */

#define VA(rva)   ((unsigned int)(IMAGE_BASE + (rva)))
#define FO(rva)   (TEXT_FILE_OFF + ((rva) - TEXT_RVA))

#define FILE_SIZE  (TEXT_FILE_OFF + (PCODE_END_RVA - TEXT_RVA))

static unsigned char pe[FILE_SIZE];

static void w16(int off, unsigned short v) {
    pe[off]   = (unsigned char)(v & 0xFF);
    pe[off+1] = (unsigned char)(v >> 8);
}
static void w32(int off, unsigned int v) {
    pe[off]   = (unsigned char)(v & 0xFF);
    pe[off+1] = (unsigned char)((v >> 8) & 0xFF);
    pe[off+2] = (unsigned char)((v >> 16) & 0xFF);
    pe[off+3] = (unsigned char)((v >> 24) & 0xFF);
}

/* Emit a byte into the P-Code region */
static int pcode_pos;
static void pb(unsigned char b) { pe[FO(PCODE_RVA) + pcode_pos++] = b; }
static void pw(unsigned short v) { pb((unsigned char)(v & 0xFF)); pb((unsigned char)(v >> 8)); }
static void pd(unsigned int v) { pb((unsigned char)(v&0xFF)); pb((unsigned char)((v>>8)&0xFF));
                                   pb((unsigned char)((v>>16)&0xFF)); pb((unsigned char)((v>>24)&0xFF)); }

int main(void)
{
    FILE *fp;
    int off;

    memset(pe, 0, sizeof(pe));
    pcode_pos = 0;

    /* ================================================================ */
    /*  DOS Header (64 bytes at offset 0, rest is stub up to 0x80)      */
    /* ================================================================ */
    w16(0x00, 0x5A4D);             /* e_magic = "MZ" */
    w16(0x02, 0x0090);             /* e_cblp (bytes on last page) */
    w16(0x04, 0x0003);             /* e_cp (pages in file) */
    w16(0x08, 0x0004);             /* e_minalloc */
    w16(0x0A, 0xFFFF);             /* e_maxalloc */
    w16(0x10, 0x00B8);             /* e_sp */
    w16(0x18, 0x0040);             /* e_lfarlc */
    w32(0x3C, 0x00000080);         /* e_lfanew -> PE header at 0x80 */

    /* ================================================================ */
    /*  PE Signature at 0x80                                            */
    /* ================================================================ */
    w32(0x80, 0x00004550);         /* "PE\0\0" */

    /* ================================================================ */
    /*  COFF File Header at 0x84 (20 bytes)                             */
    /* ================================================================ */
    w16(0x84, 0x014C);             /* Machine = IMAGE_FILE_MACHINE_I386 */
    w16(0x86, 0x0001);             /* NumberOfSections = 1 */
    w32(0x88, 0x60000000);         /* TimeDateStamp (arbitrary) */
    w32(0x8C, 0x00000000);         /* PointerToSymbolTable */
    w32(0x90, 0x00000000);         /* NumberOfSymbols */
    w16(0x94, 0x00E0);             /* SizeOfOptionalHeader = 224 */
    w16(0x96, 0x010F);             /* Characteristics: EXEC|NO_RELOC|LINE_STRIPPED|32BIT|DBG_STRIPPED */

    /* ================================================================ */
    /*  Optional Header at 0x98 (224 bytes for PE32)                    */
    /* ================================================================ */
    w16(0x98, 0x010B);             /* Magic = PE32 */
    pe[0x9A] = 8;                  /* MajorLinkerVersion */
    pe[0x9B] = 0;                  /* MinorLinkerVersion */
    w32(0x9C, PCODE_END_RVA - TEXT_RVA);  /* SizeOfCode */
    w32(0xA0, 0x00000000);         /* SizeOfInitializedData */
    w32(0xA4, 0x00000000);         /* SizeOfUninitializedData */
    w32(0xA8, EP_RVA);             /* AddressOfEntryPoint */
    w32(0xAC, TEXT_RVA);           /* BaseOfCode */
    w32(0xB0, 0x00000000);         /* BaseOfData */
    w32(0xB4, IMAGE_BASE);         /* ImageBase */
    w32(0xB8, SECT_ALIGN);         /* SectionAlignment */
    w32(0xBC, FILE_ALIGN);         /* FileAlignment */
    w16(0xC0, 4);                  /* MajorOperatingSystemVersion */
    w16(0xC2, 0);                  /* MinorOperatingSystemVersion */
    w16(0xC4, 0);                  /* MajorImageVersion */
    w16(0xC6, 0);                  /* MinorImageVersion */
    w16(0xC8, 4);                  /* MajorSubsystemVersion */
    w16(0xCA, 0);                  /* MinorSubsystemVersion */
    w32(0xCC, 0);                  /* Win32VersionValue */
    w32(0xD0, 0x00002000);         /* SizeOfImage (headers + 1 section aligned) */
    w32(0xD4, FILE_ALIGN);         /* SizeOfHeaders (file-aligned) */
    w32(0xD8, 0x00000000);         /* CheckSum */
    w16(0xDC, 3);                  /* Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI */
    w16(0xDE, 0);                  /* DllCharacteristics */
    w32(0xE0, 0x00100000);         /* SizeOfStackReserve */
    w32(0xE4, 0x00001000);         /* SizeOfStackCommit */
    w32(0xE8, 0x00100000);         /* SizeOfHeapReserve */
    w32(0xEC, 0x00001000);         /* SizeOfHeapCommit */
    w32(0xF0, 0);                  /* LoaderFlags */
    w32(0xF4, 16);                 /* NumberOfRvaAndSizes */
    /* Data directories (16 * 8 = 128 bytes) at 0xF8 */
    /* Entry 1 = Import Table */
    w32(0x100, IMPORT_DIR_RVA);        /* Import directory RVA */
    w32(0x104, 40);                    /* Size: 2 descriptors x 20 bytes */

    /* ================================================================ */
    /*  Section Header #1 (.text) at 0x178 (40 bytes)                   */
    /* ================================================================ */
    memcpy(&pe[0x178], ".text\0\0\0", 8);  /* Name */
    w32(0x180, PCODE_END_RVA - TEXT_RVA);  /* VirtualSize */
    w32(0x184, TEXT_RVA);                   /* VirtualAddress */
    w32(0x188, PCODE_END_RVA - TEXT_RVA);  /* SizeOfRawData (0x400, file-aligned) */
    w32(0x18C, TEXT_FILE_OFF);              /* PointerToRawData */
    w32(0x190, 0);                          /* PointerToRelocations */
    w32(0x194, 0);                          /* PointerToLinenumbers */
    w16(0x198, 0);                          /* NumberOfRelocations */
    w16(0x19A, 0);                          /* NumberOfLinenumbers */
    w32(0x19C, 0xE0000020);                /* Characteristics: CODE|EXEC|READ|WRITE */

    /* ================================================================ */
    /*  Entry Point Code at RVA 0x1000 (file offset 0x0200)             */
    /*  Pattern: PUSH VBHeaderVA / CALL rel32(dummy) / RET              */
    /* ================================================================ */
    off = FO(EP_RVA);
    pe[off++] = 0x68;                      /* PUSH imm32 */
    w32(off, VA(VBHDR_RVA));               /* VBHeader VA = 0x00401020 */
    off += 4;
    pe[off++] = 0xE8;                      /* CALL rel32 */
    w32(off, 0x00000000);                  /* dummy relative offset (calls next instr) */
    off += 4;
    pe[off++] = 0x58;                      /* POP EAX (clean up CALL's return address) */
    pe[off++] = 0xC3;                      /* RET */

    /* ================================================================ */
    /*  VB_HEADER at RVA 0x1020 (file offset 0x0220)                    */
    /*  #pragma pack(1) struct, total 84 bytes (0x54)                   */
    /*                                                                  */
    /*  +0x00: DWORD  dwSignature       "VB5!" = 0x21354256             */
    /*  +0x04: WORD   wRuntimeBuild                                     */
    /*  +0x06: CHAR   szLangDll[14]                                     */
    /*  +0x14: CHAR   szSecLangDll[14]                                  */
    /*  +0x22: WORD   wRuntimeRevision                                  */
    /*  +0x24: DWORD  dwLCID                                            */
    /*  +0x28: DWORD  dwSecLCID                                         */
    /*  +0x2C: DWORD  lpSubMain                                         */
    /*  +0x30: DWORD  lpProjectData    <-- pointer to VB_PROJECT_DATA   */
    /*  +0x34: DWORD  fMDLIntObjs                                       */
    /*  +0x38: DWORD  fFeatureData                                      */
    /*  +0x3C: DWORD  dwReserved[6]                                     */
    /* ================================================================ */
    off = FO(VBHDR_RVA);
    w32(off + 0x00, 0x21354256);           /* dwSignature = "VB5!" */
    w16(off + 0x04, 0x0000);               /* wRuntimeBuild */
    /* szLangDll[14] and szSecLangDll[14] left as zeros */
    w16(off + 0x22, 0x0000);               /* wRuntimeRevision */
    w32(off + 0x24, 0x00000409);           /* dwLCID = English */
    w32(off + 0x28, 0x00000409);           /* dwSecLCID */
    w32(off + 0x2C, 0x00000000);           /* lpSubMain = 0 (no Sub Main) */
    w32(off + 0x30, VA(PROJDATA_RVA));     /* lpProjectData -> VB_PROJECT_DATA */
    w32(off + 0x34, 0x00000000);           /* fMDLIntObjs */
    w32(off + 0x38, 0x00000000);           /* fFeatureData */
    /* dwReserved[6] left as zeros */

    /* ================================================================ */
    /*  VB_PROJECT_DATA at RVA 0x1080 (file offset 0x0280)              */
    /*  #pragma pack(1) struct, only first 0x14 bytes matter:           */
    /*                                                                  */
    /*  +0x00: DWORD  dwVersion                                         */
    /*  +0x04: DWORD  lpObjectTable                                     */
    /*  +0x08: DWORD  dwNull                                            */
    /*  +0x0C: DWORD  lpCodeStart      <-- VA of P-Code start           */
    /*  +0x10: DWORD  lpCodeEnd        <-- VA of P-Code end             */
    /* ================================================================ */
    off = FO(PROJDATA_RVA);
    w32(off + 0x00, 0x00000001);           /* dwVersion */
    w32(off + 0x04, 0x00000000);           /* lpObjectTable = 0 (unused) */
    w32(off + 0x08, 0x00000000);           /* dwNull */
    w32(off + 0x0C, VA(PCODE_RVA));        /* lpCodeStart */
    w32(off + 0x10, VA(PCODE_END_RVA));    /* lpCodeEnd */
    w32(off + 0x14, 0x00000000);           /* dwDataSize */
    /* Rest left as zeros */

    /* ================================================================ */
    /*  Import Directory at RVA 0x10A0 (file offset 0x02A0)             */
    /*  IMAGE_IMPORT_DESCRIPTOR #1: MSVBVM60.DLL                        */
    /*  IMAGE_IMPORT_DESCRIPTOR #2: null terminator (already zeroed)     */
    /*                                                                  */
    /*  +0x00: DWORD  OriginalFirstThunk  (ILT RVA, 0 = unused)         */
    /*  +0x04: DWORD  TimeDateStamp                                     */
    /*  +0x08: DWORD  ForwarderChain                                    */
    /*  +0x0C: DWORD  Name               (RVA of DLL name string)       */
    /*  +0x10: DWORD  FirstThunk         (IAT RVA, 0 = unused)          */
    /* ================================================================ */
    off = FO(IMPORT_DIR_RVA);
    w32(off + 0x00, 0x00000000);           /* OriginalFirstThunk (unused) */
    w32(off + 0x04, 0x00000000);           /* TimeDateStamp */
    w32(off + 0x08, 0x00000000);           /* ForwarderChain */
    w32(off + 0x0C, IMPORT_NAME_RVA);      /* Name -> "MSVBVM60.DLL" */
    w32(off + 0x10, 0x00000000);           /* FirstThunk (unused) */
    /* Descriptor #2 (null terminator) is already zeros */

    /* DLL name string at RVA 0x10D0 */
    memcpy(&pe[FO(IMPORT_NAME_RVA)], "MSVBVM60.DLL", 13); /* includes '\0' */

    /* ================================================================ */
    /*  P-Code Bytecode at RVA 0x1200 (file offset 0x0400)              */
    /*  512 bytes of VB P-Code opcodes for stress testing.              */
    /*                                                                  */
    /*  Opcode table reference (from VBOpcodeTable.h):                  */
    /*    Base opcodes:  0x00-0xFA (single byte + args)                 */
    /*    Lead prefixes: 0xFB-0xFF (prefix + index + args)              */
    /* ================================================================ */

    /* --- 1-byte base opcodes (no arguments) --- */

    pb(0x03);           /* DOC (length 1) */
    pb(0x13);           /* ExitProcHresult (length 1) */
    pb(0x14);           /* ExitProc (length 1) */
    pb(0x15);           /* ExitProcI2 (length 1) */
    pb(0x16);           /* ExitProcR4 (length 1) */
    pb(0x17);           /* ExitProcR8 (length 1) */
    pb(0x18);           /* ExitProcCy (length 1) */
    pb(0x21);           /* FLdPrThis (length 1) */
    pb(0x25);           /* PopAdLdVar (length 1) */
    pb(0x2A);           /* ConcatStr (length 1) */
    pb(0x45);           /* Error (length 1) */
    pb(0x49);           /* PopAdLd4 (length 1) */
    pb(0x4A);           /* FnLenStr (length 1) */
    pb(0x4C);           /* FnLBound (length 1) */
    pb(0x50);           /* CI4Str (length 1) */
    pb(0x52);           /* Ary1StVar (length 1) */
    pb(0x53);           /* CBoolCy (length 1) */
    pb(0x55);           /* CI2Var (length 1) */
    pb(0x5A);           /* Erase (length 1) */
    pb(0x5D);           /* HardType (length 1) */
    pb(0x60);           /* CStrVarTmp (length 1) */

    /* --- Type conversion opcodes --- */
    pb(0xEF);           /* CCyI2 (length 1) */
    pb(0xF0);           /* CCyI4 (length 1) */
    pb(0xF1);           /* CCyR4 (length 1) */
    pb(0xF2);           /* CDateR8 (length 1) */

    /* --- 2-byte base opcodes (1 arg byte) --- */
    pb(0xF4); pb(0x2A); /* LitI2_Byte 0x2A (length 2) */
    pb(0xF4); pb(0x00); /* LitI2_Byte 0x00 */
    pb(0xF4); pb(0xFF); /* LitI2_Byte 0xFF */
    pb(0x02); pb(0x05); /* SelectCaseByte (length 2) */
    pb(0x00); pb(0x10); /* LargeBos (length 2) */

    /* --- 3-byte base opcodes (2 arg bytes) --- */
    pb(0x04); pw(0x0010);   /* FLdRfVar offset=0x10 (length 3) */
    pb(0x05); pw(0x0020);   /* ImpAdLdRf addr (length 3) */
    pb(0x06); pw(0x0030);   /* MemLdRfVar (length 3) */
    pb(0x08); pw(0x0008);   /* FLdPr offset (length 3) */
    pb(0x1B); pw(0x0100);   /* LitStr string_ref (length 3) */
    pb(0x1C); pw(0x000A);   /* BranchF +10 (length 3) */
    pb(0x1D); pw(0x0006);   /* BranchT +6 (length 3) */
    pb(0x1E); pw(0x0020);   /* Branch +32 (length 3) */
    pb(0x31); pw(0x0014);   /* FStStr (length 3) */
    pb(0x46); pw(0x0008);   /* CVarStr (length 3) */
    pb(0x4B); pw(0x0040);   /* OnErrorGoto (length 3) */
    pb(0x4F); pw(0x0003);   /* MidStr (length 3) */
    pb(0x6B); pw(0x000C);   /* FLdI2 (length 3) */
    pb(0x6C); pw(0x0004);   /* ILdRf (length 3) */
    pb(0x70); pw(0x0010);   /* FStI2 (length 3) */
    pb(0x71); pw(0x0014);   /* FStR4 (length 3) */
    pb(0x72); pw(0x0018);   /* FStR8 (length 3) */
    pb(0xF3); pw(0x002A);   /* LitI2 42 (length 3) */
    pb(0xF3); pw(0xFFFF);   /* LitI2 -1 (length 3) */
    pb(0xF3); pw(0x0000);   /* LitI2 0 (length 3) */
    pb(0xF8); pw(0x4000);   /* LitI2FP (length 3) */

    /* --- 5-byte base opcodes (4 arg bytes) --- */
    pb(0x09); pd(0x00000010); /* ImpAdCallHresult (length 5) */
    pb(0x0A); pd(0x00020001); /* ImpAdCallFPR4 (length 5) */
    pb(0x0B); pd(0x00000030); /* ImpAdCallI2 (length 5) */
    pb(0x0C); pd(0x00000040); /* ImpAdCallCy (length 5) */
    pb(0x0D); pd(0x00080004); /* VCallHresult vtable+index (length 5) */
    pb(0x10); pd(0x00100002); /* ThisVCallHresult (length 5) */
    pb(0x28); pd(0x0008002A); /* LitVarI2 offset + value (length 5) */
    pb(0x64); pd(0x00040002); /* NextI2 (length 5) */
    pb(0x66); pd(0x00080003); /* NextI4 (length 5) */
    pb(0xF5); pd(0x0000BEEF); /* LitI4 0xBEEF (length 5) */
    pb(0xF5); pd(0xDEADCAFE); /* LitI4 0xDEADCAFE (length 5) */
    pb(0xF5); pd(0x00000000); /* LitI4 0 (length 5) */
    pb(0xF5); pd(0xFFFFFFFF); /* LitI4 -1 (length 5) */
    pb(0xF7); pd(0x00010000); /* LitCy4 (length 5) */
    pb(0xF9); pd(0x42280000); /* LitR4FP 42.0f (length 5) */

    /* --- 7-byte base opcodes --- */
    pb(0x61); pd(0x00040008); pw(0x0002); /* LateIdLdVar (length 7) */
    pb(0x07); pd(0x00000050); /* FMemLdRf (length 5) */

    /* --- 9-byte base opcodes --- */
    pb(0xF6); pd(0x00000000); pd(0x00010000); /* LitCy (length 9) */
    pb(0xFA); pd(0x00000000); pd(0x40E38580); /* LitDate (length 9) */

    /* --- Extended opcodes: Lead0 prefix (0xFB) --- */
    pb(0xFB); pb(0x00);           /* DOC (Lead0, length 1) */
    pb(0xFB); pb(0x01);           /* ImpUI1 (Lead0, length 1) */
    pb(0xFB); pb(0x02);           /* ImpI4 (Lead0, length 1) */
    pb(0xFB); pb(0x09);           /* EqvUI1 (Lead0, length 1) */
    pb(0xFB); pb(0x0A);           /* EqvI4 (Lead0, length 1) */
    pb(0xFB); pb(0x11);           /* XorI4 (Lead0, length 1) */
    pb(0xFB); pb(0x19);           /* OrI2 (Lead0, length 1) */
    pb(0xFB); pb(0x07); pw(0x0010); /* ImpVar (Lead0, length 3) */
    pb(0xFB); pb(0x0F); pw(0x0008); /* EqvVar (Lead0, length 3) */
    pb(0xFB); pb(0x17); pw(0x000C); /* XorVar (Lead0, length 3) */

    /* --- Extended opcodes: Lead1 prefix (0xFC) --- */
    pb(0xFC); pb(0x00);           /* Lead1 0x00 */
    pb(0xFC); pb(0x10);           /* Lead1 0x10 */
    pb(0xFC); pb(0x20);           /* Lead1 0x20 */
    pb(0xFC); pb(0x40);           /* Lead1 0x40 */

    /* --- Extended opcodes: Lead2 prefix (0xFD) --- */
    pb(0xFD); pb(0x00);           /* Lead2 0x00 */
    pb(0xFD); pb(0x10);           /* Lead2 0x10 */
    pb(0xFD); pb(0x30);           /* Lead2 0x30 */
    pb(0xFD); pb(0x50);           /* Lead2 0x50 */

    /* --- Extended opcodes: Lead3 prefix (0xFE) --- */
    pb(0xFE); pb(0x00);           /* Lead3 0x00 */
    pb(0xFE); pb(0x10);           /* Lead3 0x10 */
    pb(0xFE); pb(0x20);           /* Lead3 0x20 */

    /* --- Extended opcodes: Lead4 prefix (0xFF) --- */
    pb(0xFF); pb(0x00);           /* Lead4 0x00 */
    pb(0xFF); pb(0x10);           /* Lead4 0x10 */
    pb(0xFF); pb(0x20);           /* Lead4 0x20 */
    pb(0xFF); pb(0x30);           /* Lead4 0x30 */

    /* --- Fill remaining P-Code space with ExitProc (0x14) --- */
    while (pcode_pos < (PCODE_END_RVA - PCODE_RVA))
        pb(0x14);  /* ExitProc */

    /* ================================================================ */
    /*  Write the PE file                                               */
    /* ================================================================ */
    fp = fopen("vb_pcode.exe", "wb");
    if (!fp) {
        printf("Error: cannot create vb_pcode.exe\n");
        return 1;
    }
    fwrite(pe, 1, sizeof(pe), fp);
    fclose(fp);

    printf("vb_pcode.exe written (%d bytes)\n", (int)sizeof(pe));
    printf("  PE ImageBase:    0x%08X\n", IMAGE_BASE);
    printf("  Entry point RVA: 0x%08X\n", EP_RVA);
    printf("  VB_HEADER VA:    0x%08X\n", VA(VBHDR_RVA));
    printf("  P-Code region:   0x%08X - 0x%08X (%d bytes)\n",
           VA(PCODE_RVA), VA(PCODE_END_RVA),
           PCODE_END_RVA - PCODE_RVA);
    printf("\nLoad vb_pcode.exe in PVDasm and select VB P-Code CPU mode.\n");
    return 0;
}
