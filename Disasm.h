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

 Written by Shany Golan.
 In January, 2003.
 I have investigated P.E. file format as thoroughly as possible,
 But I cannot claim that I am an expert yet, so some of its information  
 May give you wrong results.

 Language used: Visual C++ 6.0
 Date of creation: July 06, 2002

 Date of first release: unknown ??, 2003

 You can contact me: e-mail address: Shanytc@yahoo.com

 Copyright (C) 2011. By Shany Golan.

 Permission is granted to make and distribute verbatim copies of this
 Program provided the copyright notice and this permission notice are
 Preserved on all copies.


 File: Disasm.h (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#ifndef _DISASM_H_
#define _DISASM_H_

// ================================================================
// ==========================  INCLUDES  ==========================
// ================================================================

#include <stdio.h>
#include <windows.h>
#include <string> // changed from <string.h>
#include <conio.h>
#include "Functions.h"
#include <commctrl.h>
#include "resource\resource.h"
#include <cctype> // transform operation in stl (lower/upper strings)
// ================================================================
// =========================  DEFINES  ============================
// ================================================================

#define DISASM_VER		"PVDasm Engine Core v1.5"
#define EP				"; ===========[ Program Entry Point ]==========="
#define SUBROUTINE		"; ===========[ Subroutine ]==========="
#define SUBROUTINE_END	"; ===========[ End Of Subroutine ]==========="
//#define BYTES_TO_DECODE  16
#define NE_FORMAT		"New Executable (NE) for PC"
#define PE_FORMAT		"Portable Executable (PE) for PC"
#define PE64_FORMAT		"Portable Executable 64Bit (PE+) for PC"
#define MZ_FORMAT		"MS-DOS Executable for PC"
#define BINARY_FORMAT	"Binary File"
#define x86_PC_16Bit	"80x86 (16Bit) Proccessor: PC"
#define x86_PC			"80x86 (32Bit) Proccessor: PC"
#define x87_PC			"80x87 (62Bit) Proccessor: PC"
#define CPU_chip8		"Chip8/SCHIP Processor"

#define x86			0
#define chip8		1
#define x87			2
#define x86_16Bit	3

#define MY_MSG 101
#define PV_TRUE  1
#define PV_FALSE 0 

// Register(s) Size
#define REG8  0
#define REG16 1
#define REG32 2
#define FPU   3 // Not in use.

// 8Bit Registers
#define  REG_AL 0
#define  REG_CL 1
#define  REG_DL 2
#define  REG_BL 3
#define  REG_AH 4
#define  REG_CH 5
#define  REG_DH 6
#define  REG_BH 7

// 16Bit Registers
#define  REG_AX 0
#define  REG_CX 1
#define  REG_DX 2
#define  REG_BX 3
#define  REG_SP 4
#define  REG_BP 5
#define  REG_SI 6
#define  REG_DI 7

// 32bit Registers
#define  REG_EAX 0 
#define  REG_ECX 1
#define  REG_EDX 2
#define  REG_EBX 3
#define  REG_ESP 4
#define  REG_EBP 5
#define  REG_ESI 6
#define  REG_EDI 7

// Segments
#define SEG_ES 0
#define SEG_CS 1
#define SEG_SS 2
#define SEG_DS 3
#define SEG_FS 4
#define SEG_GS 5

// SIB extention
#define SIB_EX 4 

//Jump Size
#define NEAR_JUMP  0
#define SHORT_JUMP 1

// ================================================================
// ==========================  STRUCTS  ===========================
// ================================================================

typedef struct Code_Flow{

    bool Jump;			// Instruction is a Jcc / jmp
    bool Call;			// Instruction is a Call
    bool BranchSize;	// Short / Near
    bool Ret;			// Instruction is Ret

} CODE_FLOW;

typedef struct Code_Branch{

    DWORD_PTR	Current_Index;		// Index of Call/Jxx in disasm
    DWORD_PTR	Branch_Destination;	// Destination of Jump/Call
    CODE_FLOW	BranchFlow;

} CODE_BRANCH;

typedef struct Decoded{
	
	// Define Decoded instruction struct

    DWORD_PTR	Address;		// Current address of decoded instruction
    CODE_FLOW	CodeFlow;		// InstructionS: Jump or Call 
    DWORD_PTR	OpcodeSize;		// Opcode Size
	BYTE		PrefixSize;		// Size of all prefixes used
    char		Assembly[128];	// Mnemonics
    char		Remarks[256];	// Mnemonic add ons
    char		Opcode[25];		// Opcode Byte forms

} DISASSEMBLY;


typedef struct Options{
    
    // Define Disassembly Visual Options struct
    
	DWORD_PTR  CPU;					// CPU Genre
    bool DecodePE;				// Loaded PE File to Disasm
    bool UpperCased_Disasm;		// Upper Case assembly, default is lower case
    bool DecodeBinary;			// No PE, Binary show
    bool ShowAddr;				// Show Addresses in decoding ?
    bool ShowOpcodes;			// Show Opcodes in decoding?
    bool ShowAssembly;			// Show Assembly in decoding?
    bool ShowRemarks;			// Show Remarks in decoding?
    bool ShowSections;			// Show sections in binary mode
    bool ForceDisasmBeforeEP;	// Force Disasm Bytes Upto bytes from Eentry Point
    bool StartFromEP;			// Start Disasm from entry Point (Auto Disable Forcing)
    bool ShowResolvedApis;		// Show Resolved Apis in debug output
    bool AutoDB;				// Convert 0000 Opcode to DB 0
    bool FirstPass;				// Enable FirstPass Analyze
	bool Signatures;			// Enable API Parameter Signatures
    
} DISASM_OPTIONS;

typedef struct FunctioInformation{
	
	// Define a Function Name, start and end.
	DWORD_PTR	FunctionStart,FunctionEnd;
	char		FunctionName[50];

} FUNCTION_INFORMATION;

// ================================================================
// =======================  PROTOTYPES  ===========================
// ================================================================

void Mod_RM_SIB(
		  DISASSEMBLY **Disasm,
		  char **Opcode,
		  DWORD_PTR pos, 
		  bool AddrPrefix, 
		  int SEG,
		  DWORD_PTR **index,
		  BYTE Bit_d, BYTE Bit_w, 
		  char *instruction,BYTE Op,
		  bool PrefixReg,
		  bool PrefixSeg,
		  bool PrefixAddr
		 );

void Mod_11_RM(
               BYTE d, 
               BYTE w,
               char **Opcode,
               DISASSEMBLY **Disasm,
               char instruction[],
               bool PrefixReg,
               BYTE Op,
               DWORD_PTR **index);

//////////////////////////////////////////////////////////////////////////
//                New set of instructions functions
//////////////////////////////////////////////////////////////////////////

void Mod_11_RM_EX(
                  BYTE d, 
                  BYTE w,
                  char **Opcode,
                  DISASSEMBLY **Disasm,                    
                  bool PrefixReg,
                  BYTE Op,
                  DWORD_PTR **index,
                  bool RepPrefix
                  );

void Mod_RM_SIB_EX(
                   DISASSEMBLY **Disasm,
                   char **Opcode, DWORD_PTR pos, 
                   bool AddrPrefix,
                   int SEG,
                   DWORD_PTR **index,
                   BYTE Op,
                   bool PrefixReg,
                   bool PrefixSeg,                   
                   bool PrefixAddr,
                   BYTE Bit_d,
                   BYTE Bit_w,
                   bool RepPrefix
                );

void	WINAPI Disassembler	( /*LPVOID lpParam*/									); // Main Core
void	Decode				( DISASSEMBLY *Disasm,char *Opcode,DWORD_PTR *Index		);
void	ShowDecoded			( DISASSEMBLY Disasm									);
void	FlushDecoded		( DISASSEMBLY *Disasm									);
void	GetDisasmOptions	( HWND hWnd, DISASM_OPTIONS *disopt						);
void	IntializeDisasm		( bool PeLoaded,bool PeLoaded64,bool NeLoaded,HWND myhWnd,char *File,char *FileName	);
void	GetInstruction		( BYTE Opcode,char *menemonic							);
void	GetJumpInstruction	( BYTE Opcode,char *menemonic							);
void	SaveDecoded			( DISASSEMBLY disasm,DISASM_OPTIONS dops,DWORD Index	);
void	AnalyzeInstruction	( const char* assembly, const char* existingRemarks, char* outComment, size_t outSize );
void	LoadApiSignature	(														);
void	LocateXrefs			(														);
void	LoadMapFile			( HWND hWnd												);
void	FirstPass			( DWORD_PTR Index,DWORD_PTR NumOfDecode,DWORD_PTR epAddress,char* Linear,DWORD_PTR StartSection,DWORD_PTR EndSection );
bool	GetAPIName			( char *Api_Name										);
bool	GetStringXRef		( char *xRef,DWORD_PTR Disasm_Address					);
bool	CheckDataAtAddress	( DWORD_PTR Address										);
DWORD	StringToDword		( char *Text											);
WORD	StringToWord		( char *Text											);
int		GetNewInstruction	( BYTE Op,char *ASM,bool RegPrefix,char *Opcode, DWORD_PTR Index	);
BOOL  CALLBACK DataSegmentsDlgProc			( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL  CALLBACK DisasmDlgProc				( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL  CALLBACK AdvancedOptionsDlgProc		( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL  CALLBACK FunctionsEPSegmentsDlgProc	( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
#endif