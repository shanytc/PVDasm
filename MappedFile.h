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


 File: MappedFile.h (main)
 This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#ifndef __MAPPED_FILE_H__
#define __MAPPED_FILE_H__

//////////////////////////////////////////////////////////////////////////
//							INCLUDES									//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include "x64.h"
#include "resource\resource.h"
#include "PE_Editor.h"
#include "Functions.h"
#include <iostream>
#include "Resize\AnchorResizing.h"
#include "Process.h"
#include "Disasm.h"
#include "loadpic/loadpic.h"
#include "CDisasmData.h"
#include "HexEditor.h"
#include "Patcher.h"
#include "Project.h"
#include "SDK/SDK.H"
#include "Wizard.h"
#include "PVScript/pvscript.h"

// STL Includes
#include <vector>
#include <map>

using namespace std;  // Use MicroSoft STL Templates

// ================================================================
// =======================  TYPE DEFINES  =========================
// ================================================================

typedef vector<CDisasmData>					DisasmDataArray;
typedef DisasmDataArray::iterator			ItrDisasm;
typedef vector<int>							DisasmImportsArray,IntArray;
typedef vector<CODE_BRANCH>					CodeBranch;
typedef vector<FUNCTION_INFORMATION>		FunctionInfo;
typedef FunctionInfo::iterator				FunctionInfoItr;
typedef DisasmImportsArray::iterator		ItrImports;
typedef CodeBranch::iterator				ItrCodeBranch;
typedef vector<BRANCH_DATA>					BranchData;
typedef multimap<const int, int>			XRef,MapTree;
typedef pair<const int const, int>			XrefAdd,MapTreeAdd;
typedef XRef::iterator						ItrXref;
typedef MapTree::iterator					TreeMapItr;

// ================================================================
// =========================  DEFINES  ============================
// ================================================================

#define PVDASM "PVDasm v1.7d Program Disassembler"
#define WIN32_LEAN_AND_MEAN // Discard old includes

#define AddNewData(key,data) DataAddersses.insert(MapTreeAdd(key,data));
#define AddFunction(Function) fFunctionInfo.insert(fFunctionInfo.end(),Function)

// Use MMX
#ifndef _M_X64
	#define USE_MMX // enable on 32bit version only
#endif

#ifdef USE_MMX
	#define StringLen(str) xlstrlen(str) // new lstrlen with mmx function
#else
    #define StringLen(str) lstrlen(str) // old c library strlen
#endif

#define Stringize( L )#L
#define MakeString( M, L )M(L)
#define $Line MakeString( Stringize, __LINE__ )
#define Reminder __FILE__ "(Ln " $Line "): "
//#pragma message(Reminder "Decoding")

// ================================================================
// =========================  FUNCTIONS  ==========================
// ================================================================
int		Open				( HWND hWnd					);
int		DumpPE				( HWND hWnd, char *memPtr	);
void	DebugPE				(							);
void	DebugNonPE			( HWND hWnd					);
void	ErrorHandling		( HWND hWnd					);
void	EnableID			( HWND hWnd					);
void	FreeMemory			( HWND hWnd					);
void	CloseLoadedFile		( HWND hWnd					);
void	ExitPVDasm			( HWND HWND					);
bool	CloseFiles			( HWND hWnd					);
LRESULT ProcessCustomDraw	( LPARAM lParam				);

// ================================================================
// =========================  DIALOGS  ============================
// ================================================================

BOOL CALLBACK GotoAddrressDlgProc	( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK SetCommentDlgProc		( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK AboutDlgProc			( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK ImportsDlgProc		( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK StringRefDlgProc		( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK DisasmColorsDlgProc	( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK XrefDlgProc			( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK FindDlgProc			( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK ToolTipDialog			( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam );

//////////////////////////////////////////////////////////////////////////
//							EXTERN DIALOGS								//
//////////////////////////////////////////////////////////////////////////
extern BOOL CALLBACK DataSegmentsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK FunctionsEPSegmentsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

// ================================================================
// =========================  SUB CLASSES  ========================
// ================================================================

LRESULT CALLBACK ImportSearchProc		( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK StrRefSearchProc		( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK AboutDialogSubClass	( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ListViewSubClass		( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ListViewXrefSubClass	( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ToolTipSubClass		( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
#endif