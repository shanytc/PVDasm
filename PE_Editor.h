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


 File: Pe_Editor.h (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/
#ifndef __PE_EDITOR_H__
#define __PE_EDITOR_H__

// ================================================================
// =========================  DEFINES  ============================
// ================================================================
#define IMAGE_DIRECTORY_ENTRY_EXPORT			0  // Import Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT			1  // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE			2  // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION			3  // Security Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY			4  // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_BASERELOC			5  // Debug Directory
#define IMAGE_DIRECTORY_ENTRY_DEBUG				6  // Description String
#define IMAGE_DIRECTORY_ENTRY_COPYRIGHT			7  // Machine Value (MIPS GP)
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR			8  // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_TLS				9  // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG		10 // Bound Import
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT		11 // Import Table Entry
#define IMAGE_DIRECTORY_ENTRY_IAT				12 // IAT (Import Adress Table)
#define MakePtr(cast,ptr,addValue) (cast)((DWORD)(ptr)+(DWORD)(addValue))

// ================================================================
// =========================  FUNCTIONS  ==========================
// ================================================================
void	Update					( char *MapData,HWND hWndParent,HINSTANCE hInst );
void	ShowPeData				( HWND hWnd										);
void	UpdatePE				( HWND hWnd										);
void	RebuildPE				( HWND hWnd										);
void	Update_Pe_Directories	( HWND hWnd										);
void	ShowImports				( HWND hWnd										); // Show imported funtions from dlls.
void	ShowExports				( HWND hWnd										); // Show exe function exports.
void	ClearIMpex				( HWND hWnd,char *msg							); // clear imports/exports tree items with error message.
DWORD	GetNumberOfExports		( char* FilePtr									); // Get the number of exports (for the SDK).
void	GetExportName			( char* FilePtr, DWORD Index, char* ExportName	); // Get name of exported name by Index.
DWORD	GetExportDisasmIndex	( char* FilePtr, DWORD Index					); // Get Index of the export in the disassembly view.
DWORD	GetExportOrdinal		( char* FilePtr, DWORD Index					); // Get ordinal of the export by Index.
// ================================================================
// =========================  DIALOGS  ============================
// ================================================================
BOOL CALLBACK PE_Editor     ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK PE_Directory  ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK Imports_Table ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

#endif