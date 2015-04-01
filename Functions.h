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
 In januar, 2003.
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


 File: Functions.h (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <WINDOWS.H>
#include <commctrl.h>
#include "x64.h"
// ================================================================
// =========================  STRUCTS  ============================
// ================================================================
typedef struct BrachData{

    DWORD ItemIndex;
    bool  Call;
    bool  Jump;

}BRANCH_DATA;


// Struct Used for the Saving & Loading Projects
typedef struct SaveLoadData{
    char	Address[10];		// Address
    char	Assembly[50];		// Menemonics
    char	Remarks[80];		// Menemonic addons
    char	Opcode[25];			// Opcode Byte forms
	char	Reference[128];		// References
    DWORD	SizeOfInstruction;	// Instruction's Size
    DWORD	SizeOfPrefix;		// Prefix's Size

}SAVE_DATA,LOAD_DATA;

// ================================================================
// =========================  DEFINES  ============================
// ================================================================
#define SOFTICE 0
#define IDA 1
#define OLLYDBG 2
#define W32DASM 3
#define CUSTOM 4
#define MAX_PROJECT_FILES 9
#define TAB_SIZE 1
#define TAB_CHARACTER "\t"

#define AddNewDataDefine(key,data) DataAddersses.insert(pair<const int const, int>(key,data));
#define AddNewFunction(Function) fFunctionInfo.insert(fFunctionInfo.end(),Function)
// ================================================================
// =========================  FUNCTIONS  ==========================
// ================================================================
WORD    StringToWord				( char *Text													);
DWORD   StringToDword				( char *Text													);
__int64 StringToQword				( char *Text													);
DWORD_PTR   SearchItemText			( HWND hList,char *SearchStr									);  // Function search item's text in the listview and return it's index.
DWORD_PTR   GetLastItem				( HWND hList													);  // Returns last item's index from a listview
DWORD_PTR   GetBranchDestination	( HWND hWnd														);
void    SelectLastItem				( HWND hList													);  // Selects Last Item in the list view
void    ChangeDisasmScheme			( DWORD_PTR iIndex												);  // Change Schme colors of disassembly
void    SoftIce_Schem				(																);
void    OllyDbg_Schem				(																);
void    IDA_Schem					(																);
void    W32dasm_Schem				(																);
void    Custom_Schem				(																);
void    SwapDword					( BYTE *MemPtr,DWORD_PTR *Original,DWORD_PTR* Mirrored			);
void    SwapWord					( BYTE *MemPtr,WORD *Original,WORD* Mirrored					);
int     xlstrlen					( const char* string											);
void    SetFont						( HWND hWnd														);
void    InitDisasmWindow			( HWND hWnd														);
void    OutDebug					( HWND hWnd,char* debug											);
void    SelectItem					( HWND hList,DWORD_PTR Index									);  // selects item (highlight) in the listview.
void    GetExeName					( char *FileName												);
void    Clear						( HWND hList													);
void    GotoBranch					(																); // Tracing Forward
void    BackTrace					(																); // Tracing Back
BOOL    GetXReferences				( HWND hWnd														);
BOOL	DefineAddressesAsData		( DWORD_PTR dwStart, DWORD_PTR dwEnd, BOOL mDisplayError		);
BOOL	DefineAddressesAsFunction	( DWORD_PTR dwStart, DWORD_PTR dwEnd, char* FName, BOOL mDisplayError	);
void    DockDebug					( HWND DestHWND, HWND hWndSrc,HWND hWnd							);
void    UnDockDebug					( HWND DestHWND, HWND hWndSrc,HWND hWnd							);
LONG    HitColumn					( HWND hWnd, DWORD dimx											);
LONG    HitColumnEx					( HWND hWnd,DWORD dimx,DWORD Columns							);
DWORD   HitRow						( HWND hWnd,LVHITTESTINFO *HitInfo								);
LONG    SetColor					( long handle													);
HWND    CreateToolBar				( HWND hWnd, HINSTANCE hInst									);
int     Check_HexString				( char *Bytes													);
BOOL    MaskEditControl				( HWND hwndEdit, char szMask[], DWORD nOptions					);
char*   ExtractFilePath				( char *cpFilePath												);
char*   CopyFromDisasm				(																);
void    FreeDisasmData				(																);
void    CopyToClipboard				( char *stringbuffer, HWND window								);
void    SelectAllItems				( HWND hList													);
void    CopyDisasmToClipboard		(																);
void    CopyDisasmToFile			(																);
void    GetSelectedLines			( DWORD_PTR* Start,DWORD_PTR* End								);
void    ProduceDisasm				(																);
void	ExportMapFile				(																);
DWORD_PTR   GetBranchDestByIndex	( DWORD_PTR Index												);
DWORD_PTR   SearchText				( DWORD_PTR dwPos,char* cpText,HWND hListView,bool bMatchCase,bool WholeWord	);
PIMAGE_SECTION_HEADER RVAToOffset	( IMAGE_NT_HEADERS *nt,IMAGE_SECTION_HEADER *sec,DWORD_PTR RVA,bool *Err		);
PIMAGE_SECTION_HEADER RVAToOffset64 ( IMAGE_NT_HEADERS64 *nt,IMAGE_SECTION_HEADER *sec,DWORD_PTR RVA,bool *Err		);

// ================================================================
// =========================  SUB CLASS  ==========================
// ================================================================
LRESULT CALLBACK MaskedEditProc ( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

#endif