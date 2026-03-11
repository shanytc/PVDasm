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


 File: PeEditor.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/

#include <windows.h>
#include "PE_Editor.h"
#include "PE_Sections.h"
#include "Functions.h"
#include "resource\resource.h"
#include "Resize\AnchorResizing.h"
#include "MappedFile.h"
#include <stdio.h>
#include <commctrl.h>

// ================================================================
// ==================== EXTERNAL VARIABLES ========================
// ================================================================
extern bool	LoadedPe;
extern bool LoadedPe64;
extern DWORD_PTR hFileSize;
extern DisasmDataArray	DisasmDataLines;

// ==============================================================
// ==================== GLOBAL VARIABLES ========================
// ==============================================================
IMAGE_DOS_HEADER		*dos_header;
IMAGE_NT_HEADERS		*nt_header;
IMAGE_NT_HEADERS64		*nt_header64;

IMAGE_SECTION_HEADER	*section_header;
HWND					ParenthWnd;
HINSTANCE				hInstance;
char					*FileMappedData=""; // data pointer
char					pedump[256];


// ==============================================================
// ========================= DIALOGS ============================
// ==============================================================

BOOL CALLBACK PE_Editor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // This dialog/Window shows the basic PE information

	switch (message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: 
		{
			// mask the edit controls in our window
			MaskEditControl(GetDlgItem(hWnd,IDC_NUMOFSECTIONS),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_MAGIC),				"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_SIZEOFIMAGE),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_FILEALLIGMENT),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_SIZEOFCODE),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_SUBSYSTEM),			"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_BASEOFCODE),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_SIZEOFHEADERS),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_TIMEANDDATE),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_CHARACTERISTICS),	"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_IMAGEBASE),			"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_CHECKSUM),			"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_ENTRYPOINT),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_NUMBEROFRVA),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_BASEOFDATA),		"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_SECALIGN),			"0123456789abcdefABCDEF\b",TRUE);
			
			// Limit the number of chars in the edit boxes
			SendDlgItemMessage(hWnd,IDC_NUMOFSECTIONS,		EM_SETLIMITTEXT,	(WPARAM)4,0);
			SendDlgItemMessage(hWnd,IDC_MAGIC,				EM_SETLIMITTEXT,	(WPARAM)4,0);
			SendDlgItemMessage(hWnd,IDC_SIZEOFIMAGE,		EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_FILEALLIGMENT,		EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_SIZEOFCODE,			EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_SUBSYSTEM,			EM_SETLIMITTEXT,	(WPARAM)4,0);
			SendDlgItemMessage(hWnd,IDC_BASEOFCODE,			EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_SIZEOFHEADERS,		EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_TIMEANDDATE,		EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_CHARACTERISTICS,	EM_SETLIMITTEXT,	(WPARAM)4,0);
			if(LoadedPe==TRUE){
				SendDlgItemMessage(hWnd,IDC_IMAGEBASE,EM_SETLIMITTEXT,(WPARAM)8,0);
			}
			else if(LoadedPe64==TRUE)
			{
				SendDlgItemMessage(hWnd,IDC_IMAGEBASE,EM_SETLIMITTEXT,(WPARAM)16,0);
			}
			SendDlgItemMessage(hWnd,IDC_CHECKSUM,			EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_ENTRYPOINT,			EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_NUMBEROFRVA,		EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_BASEOFDATA,			EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_SECALIGN,			EM_SETLIMITTEXT,	(WPARAM)8,0);
			
			// Show PE information
			ShowPeData(hWnd); // send the specific window Handler!
			return true;
		}
		break;
		
		case WM_LBUTTONDOWN:
		{
			// on mouse down, move the dialog
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
		
		case WM_PAINT:
		{
			// we are not painting anything!
			return false;
		}
		break;
		
		case WM_COMMAND: // our buttons
		{
			switch (LOWORD(wParam)) 
			{
				case IDC_EXIT:
				{
					if(LoadedPe==TRUE){
						OutDebug(ParenthWnd,"-> PE Editor Closed!");
					}
					else if(LoadedPe64==TRUE)
					{
						OutDebug(ParenthWnd,"-> PE+ Editor Closed!");
					}
					
                    SelectLastItem(GetDlgItem(ParenthWnd,IDC_LIST)); // Selects the Last Item
					// close the dialog
					EndDialog(hWnd,0); 
				}
				break;
				
				case IDC_SAVEPE:
				{
					// save current changes to the PE Header
					UpdatePE(hWnd);
					
					if(LoadedPe==TRUE){
						OutDebug(ParenthWnd,"-> PE Header Updated!");
					}
					else if(LoadedPe64==TRUE)
					{
						OutDebug(ParenthWnd,"-> PE+ Header Updated!");
					}

                    SelectLastItem(GetDlgItem(ParenthWnd,IDC_LIST)); // Selects the Last Item
					// close the dialog
					//EndDialog(hWnd,0);
				}
				break;

				case IDC_SECTIONS:
				{
					if(LoadedPe==TRUE || LoadedPe64==TRUE){
						// Create Linked List and copy PE sections
						Set_PE_Info(FileMappedData,hInstance);
						// Show Sections Dialog
						DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_SECTION_EDITOR), hWnd, (DLGPROC)PE_Sections);
					}
				}
				break;

				case IDC_SEC_DIRECTORY:
				{
					// Show PE directories Information Dialog
					if(LoadedPe==TRUE || LoadedPe64==TRUE){
						DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PE_DIRECTORY), hWnd, (DLGPROC)PE_Directory);
					}
				}
				break;

				case IDC_PE_IMPORTS:
				{
					// Show EXE Imports!
					if(LoadedPe==TRUE || LoadedPe64==TRUE){
						DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PE_IMPORTS), hWnd, (DLGPROC)Imports_Table);
					}
				}
				break;

				case IDC_REBUILD_PE:
				{
					// Call to the Pe Rebuilder
					if(LoadedPe==TRUE || LoadedPe64==TRUE){
						RebuildPE(ParenthWnd);
						// Refresh the PE Editor dialog with fixed values
						ShowPeData(hWnd);
					}
				}
				break;
			}
			break;
			
			case WM_CLOSE:
			{
				// Close the Current Dialog
				EndDialog(hWnd,0); 
			}
			break;
		}
		break;
	}
	return 0;
}


// ===========================================================
// =================== Imports/Exports Dialog ================
// ===========================================================
// This Dialog shows the Imports and Exports used in the PE header
BOOL CALLBACK Imports_Table(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: 
		{
			// Intialize the Tree control
			InitCommonControls();

			// Make the window resizble with a specific properties
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_IMPORTS_TREE), GWL_USERDATA,ANCHOR_RIGHT|ANCHOR_BOTTOM);
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_EXPORTS_TREE), GWL_USERDATA,ANCHOR_RIGHT|ANCHOR_BOTTOM|ANCHOR_NOT_TOP);
			
			// Show Imports
			ShowImports(hWnd);
			// Show Exports
			ShowExports(hWnd);
			// update the window after showing the imports
			UpdateWindow(hWnd);
			
			return true; // always return true
		}
		break;
		
		// Window Resize Intialize
		case WM_SHOWWINDOW:
		{	
			// Intialize the window when first showed
			InitializeResizeControls(hWnd);
		}
		break;
		
		case WM_SIZE:// Window's Controls Resize 
		{
			// Now we can resize our window
			ResizeControls(hWnd);
		}
		break;

		case WM_LBUTTONDOWN:
		{
			// Moving the window when pressing left button
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
		
		case WM_PAINT:
		{
			// We are not painting anything!
			return false;
		}
		break;	

		case WM_CLOSE:
		{
			/* 
			   Reintialize the main window
			   This must be done in order to restore the
			   Old settings of the Disasm Main Window
			*/
			InitializeResizeControls(ParenthWnd);
			// Close the dialog
			EndDialog(hWnd,0); 
		}
		break;

	}
	return 0;
}

// ==================================================================
// ========================== DIRECTORY DIALOG ======================
// ==================================================================
// This dialog shows the directory Pointers used in the PE Header
BOOL CALLBACK PE_Directory(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: 
		{
			// Mask the edit controls in our window
			// "0123456789abcdef\b chars only accepted!
			MaskEditControl( GetDlgItem( hWnd, IDC_EXPORT_RVA),			"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_EXPORT_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_IMPORT_RVA),			"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_IMPORT_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_RESOURCE_RVA),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_RESOURCE_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_EXCEPTION_RVA),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_EXCEPTION_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_SECUTIRY_RVA),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_SECUTIRY_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_BASERELOC_RVA),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_BASERELOC_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_DEBUG_RVA),			"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_DEBUG_SIZE),			"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_COPYRIGHT_RVA),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_COPYRIGHT_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_GLOBALPTR_RVA),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_GLOBALPTR_SIZE),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_TLS_RVA),			"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_TLS_SIZE),			"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_LOADCONFIG_RVA),		"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_LOADCONFIG_SIZE),	"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_BOUNDIMPORT_RVA),	"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_BOUNDIMPORT_SIZE),	"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_IAT_RVA),			"0123456789abcdefABCDEF\b", TRUE );
			MaskEditControl( GetDlgItem( hWnd, IDC_IAT_SIZE),			"0123456789abcdefABCDEF\b", TRUE );

			// Limit the number of chars in the edit boxes (8 chars!)
			SendDlgItemMessage( hWnd, IDC_EXPORT_RVA,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_EXPORT_SIZE,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_IMPORT_RVA,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_IMPORT_SIZE,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_RESOURCE_RVA,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_RESOURCE_SIZE,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_EXCEPTION_RVA,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_EXCEPTION_SIZE,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_SECUTIRY_RVA,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_SECUTIRY_SIZE,	EM_SETLIMITTEXT,    (WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_BASERELOC_RVA,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_BASERELOC_SIZE,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_DEBUG_RVA,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_DEBUG_SIZE,		EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_COPYRIGHT_RVA,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_COPYRIGHT_SIZE,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_GLOBALPTR_RVA,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_GLOBALPTR_SIZE,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_TLS_RVA,			EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_TLS_SIZE,			EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_LOADCONFIG_RVA,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_LOADCONFIG_SIZE,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_BOUNDIMPORT_RVA,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_BOUNDIMPORT_SIZE,	EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_IAT_RVA,			EM_SETLIMITTEXT,	(WPARAM)8, 0 );
			SendDlgItemMessage( hWnd, IDC_IAT_SIZE,			EM_SETLIMITTEXT,    (WPARAM)8, 0 );
		
            DWORD i;
			char Temp[10]="";
			ShowWindow(hWnd,SW_NORMAL);

			// Read/scan the directory information, and show
			// The Pointers (RVA/Sizes) at the edit-boxes.
			// Each number has a specified directory meaning.
			// i is the scaner and have a representive in the defines.

			if(LoadedPe==TRUE)
			{
				for(i=0;i<(nt_header->OptionalHeader.NumberOfRvaAndSizes);i++)
				{
				  switch(i) // what are we on?
				  {
					// Import pointer
					case IMAGE_DIRECTORY_ENTRY_IMPORT:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_IMPORT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_IMPORT_SIZE,Temp);
					}
					break;

					// Export pointer
					case IMAGE_DIRECTORY_ENTRY_EXPORT:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_EXPORT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_EXPORT_SIZE,Temp);
					}
					break;

					// Resource Pointer
					case IMAGE_DIRECTORY_ENTRY_RESOURCE:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_RESOURCE_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_RESOURCE_SIZE,Temp);
					}
					break;

					// Exception pointer
					case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_EXCEPTION_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_EXCEPTION_SIZE,Temp);
					}
					break;
					
					// Security Pointer
					case IMAGE_DIRECTORY_ENTRY_SECURITY:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_SECUTIRY_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_SECUTIRY_SIZE,Temp);
					}
					break;
					
					// BaseRelocation Poitner
					case IMAGE_DIRECTORY_ENTRY_BASERELOC:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_BASERELOC_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_BASERELOC_SIZE,Temp);
					}
					break;
					
					// Debug Pointer
					case IMAGE_DIRECTORY_ENTRY_DEBUG:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_DEBUG_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_DEBUG_SIZE,Temp);
					}
					break;
					
					// CopyRight Pointer
					case IMAGE_DIRECTORY_ENTRY_COPYRIGHT:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_COPYRIGHT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_COPYRIGHT_SIZE,Temp);
					}
					break;
					
					// GlobalPtr Pointer
					case IMAGE_DIRECTORY_ENTRY_GLOBALPTR:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_GLOBALPTR_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_GLOBALPTR_SIZE,Temp);
					}
					break;
					
					// TLS Pointer												
					case IMAGE_DIRECTORY_ENTRY_TLS:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_TLS_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_TLS_SIZE,Temp);
					}
					break;
					
					// LoadConfig Pointer
					case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_LOADCONFIG_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_LOADCONFIG_SIZE,Temp);
					}
					break;

					// Bound Import Pointer
					case IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_BOUNDIMPORT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_BOUNDIMPORT_SIZE,Temp);
					}
					break;
					
					// IAT (Import Address Table) Pointer
					case IMAGE_DIRECTORY_ENTRY_IAT:
					{
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_IAT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_IAT_SIZE,Temp);
					}
					break;
				  }
				}
			}
			else if(LoadedPe64==TRUE)
			{
				for(i=0;i<(nt_header64->OptionalHeader.NumberOfRvaAndSizes);i++)
				{
				  switch(i) // what are we on?
				  {
					// Import pointer
					case IMAGE_DIRECTORY_ENTRY_IMPORT:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_IMPORT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_IMPORT_SIZE,Temp);
					}
					break;

					// Export pointer
					case IMAGE_DIRECTORY_ENTRY_EXPORT:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_EXPORT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_EXPORT_SIZE,Temp);
					}
					break;

					// Resource Pointer
					case IMAGE_DIRECTORY_ENTRY_RESOURCE:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_RESOURCE_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_RESOURCE_SIZE,Temp);
					}
					break;

					// Exception pointer
					case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_EXCEPTION_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_EXCEPTION_SIZE,Temp);
					}
					break;
					
					// Security Pointer
					case IMAGE_DIRECTORY_ENTRY_SECURITY:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_SECUTIRY_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_SECUTIRY_SIZE,Temp);
					}
					break;
					
					// BaseRelocation Poitner
					case IMAGE_DIRECTORY_ENTRY_BASERELOC:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_BASERELOC_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_BASERELOC_SIZE,Temp);
					}
					break;
					
					// Debug Pointer
					case IMAGE_DIRECTORY_ENTRY_DEBUG:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_DEBUG_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_DEBUG_SIZE,Temp);
					}
					break;
					
					// CopyRight Pointer
					case IMAGE_DIRECTORY_ENTRY_COPYRIGHT:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_COPYRIGHT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_COPYRIGHT_SIZE,Temp);
					}
					break;
					
					// GlobalPtr Pointer
					case IMAGE_DIRECTORY_ENTRY_GLOBALPTR:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_GLOBALPTR_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_GLOBALPTR_SIZE,Temp);
					}
					break;
					
					// TLS Pointer												
					case IMAGE_DIRECTORY_ENTRY_TLS:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_TLS_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_TLS_SIZE,Temp);
					}
					break;
					
					// LoadConfig Pointer
					case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_LOADCONFIG_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_LOADCONFIG_SIZE,Temp);
					}
					break;

					// Bound Import Pointer
					case IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_BOUNDIMPORT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_BOUNDIMPORT_SIZE,Temp);
					}
					break;
					
					// IAT (Import Address Table) Pointer
					case IMAGE_DIRECTORY_ENTRY_IAT:
					{
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].VirtualAddress);
						SetDlgItemText(hWnd,IDC_IAT_RVA,Temp);
						wsprintf(Temp,"%08X",nt_header64->OptionalHeader.DataDirectory[i].Size);
						SetDlgItemText(hWnd,IDC_IAT_SIZE,Temp);
					}
					break;
				  }
				}
			}
			
			return true;
		}
		break;
		
		case WM_LBUTTONDOWN:
		{
			//  move the dialog by pressing the left mouse button
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
		
		case WM_PAINT:
		{
			// We are not Painting!
			return false;
		}
		break;
	
		case WM_CLOSE:
		{
			// Close the dialog
			EndDialog(hWnd,0); 
		}
		break;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam)) 
			{
				case IDC_DIRECTORY_CANCEL:
				{
					// Close the directory dialog
					EndDialog(hWnd,0);
				}
				break;

				case IDC_DIRECTORY_SAVE:
				{
					// Update the information to the PE header (Directory Table)
					Update_Pe_Directories(hWnd);
					// Close the dialog
					EndDialog(hWnd,0);
				}
				break;

				case IDC_SHOW_IMPORTS:
				{
					// Show the imports from the directory dialog
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PE_IMPORTS), hWnd, (DLGPROC)Imports_Table);
				}
				break;

				case IDC_SHOW_EXPORTS:
				{
					// Show the imports from the directory dialog
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PE_IMPORTS), hWnd, (DLGPROC)Imports_Table);
				}
				break;
			}
			break;
		}
		break;

	}
	return 0;
}


//==========================================================================
//=========================== FUNCTIONS ====================================
//==========================================================================

void Update(char *MapData,HWND hWndParent,HINSTANCE hInst)
{
	/*
	   Function: 
	   ---------
	   "Update"

	   Parameters:
	   -----------
	   1. char *MapData - points to the memory mapped file
	   2. HWND hWndParent - Window Handler
	   3. HINSTANCE hInst - program's instance
	   
	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   This function will retrive the memory mapped file
	   Pointer, as well as the Main Window Handle (hWnd)
	   And the program's Instance! (for the bitmaps)
	   And will pass this parameters to a global copies
	   In order to use them in the PE editor
	
	   ������� �� ����� 3 ������
	   ������ ��� ����� ����� ������ ������� ��� �����
	   ���� ���� ����� ����� ����.
	   ��� �� �������� ����� �� ����� ��
	   ����� ����� ���� �� �� ���� ����� �� �����

	*/
	
	// Get hWnd of main dialog (for debug dump)
	ParenthWnd=hWndParent; 
	// create a copy of memory pointer
	FileMappedData=MapData;
	// copy exe hinstance handler (used for bitmaps in menus)
	hInstance=hInst;
    // create PE pointers of our loaded file
	dos_header=(IMAGE_DOS_HEADER*)FileMappedData;
	if(LoadedPe==TRUE){
		nt_header=(IMAGE_NT_HEADERS*)(dos_header->e_lfanew+FileMappedData);
	}
	else if(LoadedPe64==TRUE)
	{
		nt_header64=(IMAGE_NT_HEADERS64*)(dos_header->e_lfanew+FileMappedData);
	}
}

// =====================================
// =========== Update PE  ==============
// =====================================
void UpdatePE(HWND hWnd)
{
	/*
	   Function:
	   ---------
	   "UpdatePE"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler to update

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   Update the PE header with 
	   New data the that user has changed.
	   The Function is using 2 major functions:
	   1. StringToDword
	   2. StringToWord
	   they are converting Word/Dword string to
	   A real Number (Hex) in Assembly code.

	   ����� ������� ������� �����
	   ���� ���� ��� �����

	   The Function returns Nothing!
    */

	// temp 
	DWORD SaveTemp;
	__int64 Temp64;
	WORD  SaveTemp2;

	if(LoadedPe==TRUE)
	{
		// Saving Number Of Sections
		GetDlgItemText(hWnd,IDC_NUMOFSECTIONS,pedump,5);
		// convert it to WORD
		SaveTemp2=StringToWord(pedump);
		// save it to the memory
		nt_header->FileHeader.NumberOfSections=SaveTemp2;
		
		// Saving Size of Image
		GetDlgItemText(hWnd,IDC_SIZEOFIMAGE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.SizeOfImage=SaveTemp;
		
		// Saving Size of Code
		GetDlgItemText(hWnd,IDC_SIZEOFCODE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.SizeOfCode=SaveTemp;
		
		// Saving Base Of Code
		GetDlgItemText(hWnd,IDC_BASEOFCODE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.BaseOfCode=SaveTemp;
		
		// Saving Time And Date Stamp
		GetDlgItemText(hWnd,IDC_TIMEANDDATE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->FileHeader.TimeDateStamp=SaveTemp;
		
		// Saving Image Base
		GetDlgItemText(hWnd,IDC_IMAGEBASE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.ImageBase=SaveTemp;

		// Saving the Entry Point of the EXE
		GetDlgItemText(hWnd,IDC_ENTRYPOINT,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.AddressOfEntryPoint=SaveTemp;
		
		// Saving Base Of Data
		GetDlgItemText(hWnd,IDC_BASEOFDATA,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.BaseOfData=SaveTemp;
		
		// Saving Magic
		GetDlgItemText(hWnd,IDC_MAGIC,pedump,10);
		SaveTemp2=StringToWord(pedump);
		nt_header->OptionalHeader.Magic=SaveTemp2;
		
		// Saving File Alligment
		GetDlgItemText(hWnd,IDC_FILEALLIGMENT,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.FileAlignment=SaveTemp;
		
		// Saving Sub System
		GetDlgItemText(hWnd,IDC_SUBSYSTEM,pedump,10);
		SaveTemp2=StringToWord(pedump);
		nt_header->OptionalHeader.Subsystem=SaveTemp2;
		
		// Saving Size of Headers
		GetDlgItemText(hWnd,IDC_SIZEOFHEADERS,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.SizeOfHeaders=SaveTemp;
		
		// Savign Charactaristics
		GetDlgItemText(hWnd,IDC_CHARACTERISTICS,pedump,10);
		SaveTemp2=StringToWord(pedump);
		nt_header->FileHeader.Characteristics=SaveTemp2;
		
		// Saving Check Sum
		GetDlgItemText(hWnd,IDC_CHECKSUM,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.CheckSum=SaveTemp;
		
		// Saving Number Of RVAs
		GetDlgItemText(hWnd,IDC_NUMBEROFRVA,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.NumberOfRvaAndSizes=SaveTemp;
		
		// Saving Section Alignment
		GetDlgItemText(hWnd,IDC_SECALIGN,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header->OptionalHeader.SectionAlignment=SaveTemp;
	}
	else if(LoadedPe64==TRUE)
	{
		// Saving Number Of Sections
		GetDlgItemText(hWnd,IDC_NUMOFSECTIONS,pedump,5);
		// convert it to WORD
		SaveTemp2=StringToWord(pedump);
		// save it to the memory
		nt_header64->FileHeader.NumberOfSections=SaveTemp2;
		
		// Saving Size of Image
		GetDlgItemText(hWnd,IDC_SIZEOFIMAGE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.SizeOfImage=SaveTemp;
		
		// Saving Size of Code
		GetDlgItemText(hWnd,IDC_SIZEOFCODE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.SizeOfCode=SaveTemp;
		
		// Saving Base Of Code
		GetDlgItemText(hWnd,IDC_BASEOFCODE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.BaseOfCode=SaveTemp;
		
		// Saving Time And Date Stamp
		GetDlgItemText(hWnd,IDC_TIMEANDDATE,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->FileHeader.TimeDateStamp=SaveTemp;
		
		// Saving Image Base
		GetDlgItemText(hWnd,IDC_IMAGEBASE,pedump,17);
		Temp64=StringToQword(pedump);
		nt_header64->OptionalHeader.ImageBase=Temp64;

		// Saving the Entry Point of the EXE
		GetDlgItemText(hWnd,IDC_ENTRYPOINT,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.AddressOfEntryPoint=SaveTemp;
		
		// Saving Magic
		GetDlgItemText(hWnd,IDC_MAGIC,pedump,10);
		SaveTemp2=StringToWord(pedump);
		nt_header64->OptionalHeader.Magic=SaveTemp2;
		
		// Saving File Alligment
		GetDlgItemText(hWnd,IDC_FILEALLIGMENT,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.FileAlignment=SaveTemp;
		
		// Saving Sub System
		GetDlgItemText(hWnd,IDC_SUBSYSTEM,pedump,10);
		SaveTemp2=StringToWord(pedump);
		nt_header64->OptionalHeader.Subsystem=SaveTemp2;
		
		// Saving Size of Headers
		GetDlgItemText(hWnd,IDC_SIZEOFHEADERS,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.SizeOfHeaders=SaveTemp;
		
		// Savign Charactaristics
		GetDlgItemText(hWnd,IDC_CHARACTERISTICS,pedump,10);
		SaveTemp2=StringToWord(pedump);
		nt_header64->FileHeader.Characteristics=SaveTemp2;
		
		// Saving Check Sum
		GetDlgItemText(hWnd,IDC_CHECKSUM,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.CheckSum=SaveTemp;
		
		// Saving Number Of RVAs
		GetDlgItemText(hWnd,IDC_NUMBEROFRVA,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.NumberOfRvaAndSizes=SaveTemp;
		
		// Saving Section Alignment
		GetDlgItemText(hWnd,IDC_SECALIGN,pedump,10);
		SaveTemp=StringToDword(pedump);
		nt_header64->OptionalHeader.SectionAlignment=SaveTemp;
	}
}

// ==============================================
// =========== Show PE Information ==============
// ==============================================
void ShowPeData(HWND hWnd)
{

	/* 
	   Function:
	   ---------
	   "ShowPeData"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler to update

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   This Function Shows The PE information
	   Onto the Edit Boxes when the User enters the
	   Pe Editor.

	   �������� ����� �� ������ �������� �� ��� �����
	   �� ��� ����� �����.

	   The functions returns Nothing!
	*/

	if(LoadedPe==TRUE){
		// Show Number of Sections
		wsprintf(pedump,"%04X",nt_header->FileHeader.NumberOfSections);	
		SetDlgItemText(hWnd,IDC_NUMOFSECTIONS,pedump);
		
		// Show Size of Image
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.SizeOfImage);
		SetDlgItemText(hWnd,IDC_SIZEOFIMAGE,pedump);
		
		// Show Size of Code
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.SizeOfCode);
		SetDlgItemText(hWnd,IDC_SIZEOFCODE,pedump);
		
		// Show Base Of Code
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.BaseOfCode);
		SetDlgItemText(hWnd,IDC_BASEOFCODE,pedump);
		
		// Shw Time and Date stamp
		wsprintf(pedump,"%08X",nt_header->FileHeader.TimeDateStamp);
		SetDlgItemText(hWnd,IDC_TIMEANDDATE,pedump);
		
		// Show Image Base
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.ImageBase);
		SetDlgItemText(hWnd,IDC_IMAGEBASE,pedump);
		
		// Show Entry Point
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.AddressOfEntryPoint);
		SetDlgItemText(hWnd,IDC_ENTRYPOINT,pedump);
		
		// Show Base of data
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.BaseOfData);
		SetDlgItemText(hWnd,IDC_BASEOFDATA,pedump);
		
		// Show Magic
		wsprintf(pedump,"%04X",nt_header->OptionalHeader.Magic);
		SetDlgItemText(hWnd,IDC_MAGIC,pedump);
		
		// Show File Alignment
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.FileAlignment);
		SetDlgItemText(hWnd,IDC_FILEALLIGMENT,pedump);
		
		// Show Sub-System
		wsprintf(pedump,"%04X",nt_header->OptionalHeader.Subsystem);
		SetDlgItemText(hWnd,IDC_SUBSYSTEM,pedump);
		
		// Show Size of Header
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.SizeOfHeaders);
		SetDlgItemText(hWnd,IDC_SIZEOFHEADERS,pedump);
		
		// Show charactaristics
		wsprintf(pedump,"%04X",nt_header->FileHeader.Characteristics);
		SetDlgItemText(hWnd,IDC_CHARACTERISTICS,pedump);
		
		// Show CheckSum
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.CheckSum);
		SetDlgItemText(hWnd,IDC_CHECKSUM,pedump);
		
		// Show Number of Sizes And RVAs
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.NumberOfRvaAndSizes);
		SetDlgItemText(hWnd,IDC_NUMBEROFRVA,pedump);
		
		// Show Section Alignment
		wsprintf(pedump,"%08X",nt_header->OptionalHeader.SectionAlignment);
		SetDlgItemText(hWnd,IDC_SECALIGN,pedump);
	}
	else if(LoadedPe64==TRUE)
	{
		// Show Number of Sections
		wsprintf(pedump,"%04X",nt_header64->FileHeader.NumberOfSections);	
		SetDlgItemText(hWnd,IDC_NUMOFSECTIONS,pedump);
		
		// Show Size of Image
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.SizeOfImage);
		SetDlgItemText(hWnd,IDC_SIZEOFIMAGE,pedump);
		
		// Show Size of Code
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.SizeOfCode);
		SetDlgItemText(hWnd,IDC_SIZEOFCODE,pedump);
		
		// Show Base Of Code
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.BaseOfCode);
		SetDlgItemText(hWnd,IDC_BASEOFCODE,pedump);
		
		// Shw Time and Date stamp
		wsprintf(pedump,"%08X",nt_header64->FileHeader.TimeDateStamp);
		SetDlgItemText(hWnd,IDC_TIMEANDDATE,pedump);
		
		// Show Image Base
		wsprintf(pedump,"%016I64X",nt_header64->OptionalHeader.ImageBase);
		SetDlgItemText(hWnd,IDC_IMAGEBASE,pedump);
		
		// Show Entry Point
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.AddressOfEntryPoint);
		SetDlgItemText(hWnd,IDC_ENTRYPOINT,pedump);
		
		// Show Base of data
		//wsprintf(pedump,"%08X",nt_header64->OptionalHeader.BaseOfData);
		SetDlgItemText(hWnd,IDC_BASEOFDATA,"N/A");
		EnableWindow(GetDlgItem(hWnd,IDC_BASEOFDATA),FALSE);
		
		// Show Magic
		wsprintf(pedump,"%04X",nt_header64->OptionalHeader.Magic);
		SetDlgItemText(hWnd,IDC_MAGIC,pedump);
		
		// Show File Alignment
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.FileAlignment);
		SetDlgItemText(hWnd,IDC_FILEALLIGMENT,pedump);
		
		// Show Sub-System
		wsprintf(pedump,"%04X",nt_header64->OptionalHeader.Subsystem);
		SetDlgItemText(hWnd,IDC_SUBSYSTEM,pedump);
		
		// Show Size of Header
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.SizeOfHeaders);
		SetDlgItemText(hWnd,IDC_SIZEOFHEADERS,pedump);
		
		// Show charactaristics
		wsprintf(pedump,"%04X",nt_header64->FileHeader.Characteristics);
		SetDlgItemText(hWnd,IDC_CHARACTERISTICS,pedump);
		
		// Show CheckSum
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.CheckSum);
		SetDlgItemText(hWnd,IDC_CHECKSUM,pedump);
		
		// Show Number of Sizes And RVAs
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.NumberOfRvaAndSizes);
		SetDlgItemText(hWnd,IDC_NUMBEROFRVA,pedump);
		
		// Show Section Alignment
		wsprintf(pedump,"%08X",nt_header64->OptionalHeader.SectionAlignment);
		SetDlgItemText(hWnd,IDC_SECALIGN,pedump);
	}
}


// ==============================================
// ========= Update PE Directories ==============
// ==============================================
void Update_Pe_Directories(HWND hWnd)
{
	/* 
	   Function:
	   ---------
	   "Update_Pe_Directories"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler to update

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   This Procedure updates the 
	   directory table at the PE header.
	   the function will retrieve the new
	   information from the edit boxes and will
	   write it back to the directory table.

	   ��������� ������ �� ����� ������� �� ����
	   ���� �"� ����� ������� �������� ������� ����� �������
	*/ 

	DWORD i,SaveTemp,NumberOfRvaAndSizes;	
	NumberOfRvaAndSizes=nt_header->OptionalHeader.NumberOfRvaAndSizes;
	
	// Loop all RVAs
	for(i=0;i<(NumberOfRvaAndSizes);i++)
	{
		switch(i)
		{
			// Entry Point 
			case IMAGE_DIRECTORY_ENTRY_IMPORT:
			{
				GetDlgItemText(hWnd,IDC_IMPORT_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;
				
				GetDlgItemText(hWnd,IDC_IMPORT_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// Entry Export 
			case IMAGE_DIRECTORY_ENTRY_EXPORT:
			{
				GetDlgItemText(hWnd,IDC_EXPORT_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;
				
				GetDlgItemText(hWnd,IDC_EXPORT_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// Resource
			case IMAGE_DIRECTORY_ENTRY_RESOURCE:
			{
				GetDlgItemText(hWnd,IDC_RESOURCE_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_RESOURCE_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// Exception
			case IMAGE_DIRECTORY_ENTRY_EXCEPTION:
			{
				GetDlgItemText(hWnd,IDC_EXCEPTION_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_EXCEPTION_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// Security
			case IMAGE_DIRECTORY_ENTRY_SECURITY:
			{
				GetDlgItemText(hWnd,IDC_SECUTIRY_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_SECUTIRY_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;	
			}
			break;
			
			// BaseReloc
			case IMAGE_DIRECTORY_ENTRY_BASERELOC:
			{
				GetDlgItemText(hWnd,IDC_BASERELOC_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_BASERELOC_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// Debug
			case IMAGE_DIRECTORY_ENTRY_DEBUG:
			{
				GetDlgItemText(hWnd,IDC_DEBUG_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_DEBUG_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
				
			// CopyRight
			case IMAGE_DIRECTORY_ENTRY_COPYRIGHT:
			{
				GetDlgItemText(hWnd,IDC_COPYRIGHT_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_COPYRIGHT_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// GlobalPTR
			case IMAGE_DIRECTORY_ENTRY_GLOBALPTR:
			{
				GetDlgItemText(hWnd,IDC_GLOBALPTR_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_GLOBALPTR_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// TLS
			case IMAGE_DIRECTORY_ENTRY_TLS:
			{
				GetDlgItemText(hWnd,IDC_TLS_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_TLS_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// Config
			case IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG:
			{
				GetDlgItemText(hWnd,IDC_LOADCONFIG_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_LOADCONFIG_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// Bound Import
			case IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT:
			{
				GetDlgItemText(hWnd,IDC_BOUNDIMPORT_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_BOUNDIMPORT_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
			
			// IAT
			case IMAGE_DIRECTORY_ENTRY_IAT:
			{
				GetDlgItemText(hWnd,IDC_IAT_RVA,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].VirtualAddress=SaveTemp;

				GetDlgItemText(hWnd,IDC_IAT_SIZE,pedump,9);
				SaveTemp=StringToDword(pedump);
				nt_header->OptionalHeader.DataDirectory[i].Size=SaveTemp;
			}
			break;
		}
	}
}


// =======================================
// ============= Rebuild PE ==============
// =======================================
void RebuildPE(HWND hWnd)
{
	/*
	   Function:
	   ---------
	   "RebuildPE"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler to update

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   This function reWrite new information
	   into the PE header and in the sections.
	   this is done in order to make the dumped image
	   works when we run it.
	   Enhanced to fix zeroed header fields (anti-dump protection)
	   and rebuild the IAT (Import Address Table).
	*/

	DWORD i,NumberOfSections;
	DWORD temp=0x00001000; // aligment/size of 4096 (1000h)
	char InfoText[256];

	// print debug information
	OutDebug(hWnd,"");

	if(LoadedPe==TRUE){
		OutDebug(hWnd,"-> Starting PE Rebuilding Session <-");
		OutDebug(hWnd,"============================");
	}
	else if(LoadedPe64==TRUE)
	{
		OutDebug(hWnd,"-> Starting PE+ Rebuilding Session <-");
		OutDebug(hWnd,"============================");
	}

	// =====================================================
	// Phase A: Detect & fix NumberOfSections if zero
	// =====================================================
	if(LoadedPe==TRUE){
		if(nt_header->FileHeader.NumberOfSections == 0){
			IMAGE_SECTION_HEADER *scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
			WORD detectedSections = 0;
			for(int s=0; s<96; s++){
				if(scanSec->VirtualAddress == 0 && scanSec->Misc.VirtualSize == 0)
					break;
				detectedSections++;
				scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(scanSec)+sizeof(IMAGE_SECTION_HEADER));
			}
			if(detectedSections > 0){
				wsprintf(InfoText,"Detected NumberOfSections: %d (was 0)", detectedSections);
				OutDebug(hWnd,InfoText);
				nt_header->FileHeader.NumberOfSections = detectedSections;
			}
		}
	}
	else if(LoadedPe64==TRUE)
	{
		if(nt_header64->FileHeader.NumberOfSections == 0){
			IMAGE_SECTION_HEADER *scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
			WORD detectedSections = 0;
			for(int s=0; s<96; s++){
				if(scanSec->VirtualAddress == 0 && scanSec->Misc.VirtualSize == 0)
					break;
				detectedSections++;
				scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(scanSec)+sizeof(IMAGE_SECTION_HEADER));
			}
			if(detectedSections > 0){
				wsprintf(InfoText,"Detected NumberOfSections: %d (was 0)", detectedSections);
				OutDebug(hWnd,InfoText);
				nt_header64->FileHeader.NumberOfSections = detectedSections;
			}
		}
	}

	// =====================================================
	// Phase B: Fix Magic if zero
	// =====================================================
	if(LoadedPe==TRUE){
		if(nt_header->OptionalHeader.Magic == 0){
			OutDebug(hWnd,"Fixing Magic from 0000 to 010B (PE32)");
			nt_header->OptionalHeader.Magic = 0x010B;
		}
	}
	else if(LoadedPe64==TRUE)
	{
		if(nt_header64->OptionalHeader.Magic == 0){
			OutDebug(hWnd,"Fixing Magic from 0000 to 020B (PE+)");
			nt_header64->OptionalHeader.Magic = 0x020B;
		}
	}

	// =====================================================
	// Phase C: Fix Characteristics if zero
	// =====================================================
	if(LoadedPe==TRUE){
		if(nt_header->FileHeader.Characteristics == 0){
			OutDebug(hWnd,"Fixing Characteristics from 0000 to 010F (PE32)");
			nt_header->FileHeader.Characteristics = 0x010F;
		}
	}
	else if(LoadedPe64==TRUE)
	{
		if(nt_header64->FileHeader.Characteristics == 0){
			OutDebug(hWnd,"Fixing Characteristics from 0000 to 0022 (PE+)");
			nt_header64->FileHeader.Characteristics = 0x0022;
		}
	}

	// =====================================================
	// Phase D: Fix Subsystem if zero
	// =====================================================
	if(LoadedPe==TRUE){
		if(nt_header->OptionalHeader.Subsystem == 0){
			OutDebug(hWnd,"Fixing Subsystem from 0000 to 0002 (WINDOWS_GUI)");
			nt_header->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
		}
	}
	else if(LoadedPe64==TRUE)
	{
		if(nt_header64->OptionalHeader.Subsystem == 0){
			OutDebug(hWnd,"Fixing Subsystem from 0000 to 0002 (WINDOWS_GUI)");
			nt_header64->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
		}
	}

	// =====================================================
	// Phase E: Fix SizeOfOptionalHeader if zero
	// =====================================================
	if(LoadedPe==TRUE){
		if(nt_header->FileHeader.SizeOfOptionalHeader == 0){
			wsprintf(InfoText,"Fixing SizeOfOptionalHeader from 0000 to %04X", (DWORD)sizeof(IMAGE_OPTIONAL_HEADER));
			OutDebug(hWnd,InfoText);
			nt_header->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
		}
	}
	else if(LoadedPe64==TRUE)
	{
		if(nt_header64->FileHeader.SizeOfOptionalHeader == 0){
			wsprintf(InfoText,"Fixing SizeOfOptionalHeader from 0000 to %04X", (DWORD)sizeof(IMAGE_OPTIONAL_HEADER64));
			OutDebug(hWnd,InfoText);
			nt_header64->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
		}
	}

	// =====================================================
	// Phase F: Existing section alignment (FileAlignment, SizeOfHeaders, sections)
	// =====================================================
	if(LoadedPe==TRUE){
		wsprintf(InfoText,"Overiding File Alignment from %08X to %08X",nt_header->OptionalHeader.FileAlignment,temp);
	}
	else if(LoadedPe64==TRUE)
	{
		wsprintf(InfoText,"Overiding File Alignment from %08X to %08X",nt_header64->OptionalHeader.FileAlignment,temp);
	}

	// update aligment/size of 4096 (1000h)
	if(LoadedPe==TRUE){
		nt_header->OptionalHeader.FileAlignment=temp;
		OutDebug(hWnd,InfoText);
		wsprintf(InfoText,"Overiding Size Of Headers from %08X to %08X",nt_header->OptionalHeader.SizeOfHeaders,temp);
	}
	else if(LoadedPe64==TRUE)
	{
		nt_header64->OptionalHeader.FileAlignment=temp;
		OutDebug(hWnd,InfoText);
		wsprintf(InfoText,"Overiding Size Of Headers from %08X to %08X",nt_header64->OptionalHeader.SizeOfHeaders,temp);
	}

	// update aligment/size of 4096 (1000h)
	if(LoadedPe==TRUE){
		nt_header->OptionalHeader.SizeOfHeaders=temp;
	}
	else if(LoadedPe64==TRUE)
	{
		nt_header64->OptionalHeader.SizeOfHeaders=temp;
	}

	OutDebug(hWnd,InfoText);

	// pointer to the section header
	if(LoadedPe==TRUE){
		section_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
	}
	else if(LoadedPe64==TRUE)
	{
		section_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
	}

	// print debug info
	OutDebug(hWnd,"");
	OutDebug(hWnd,"Rebuilding Sections:");

	if(LoadedPe==TRUE){
		NumberOfSections=nt_header->FileHeader.NumberOfSections;
	}
	else if(LoadedPe64==TRUE)
	{
		NumberOfSections=nt_header64->FileHeader.NumberOfSections;
	}
	// updating the sections to have same RVA and RAW information
	// Use a local pointer so we don't advance the global section_header past all sections
	IMAGE_SECTION_HEADER *sec_iter = section_header;
	for(i=0;i<NumberOfSections;i++)
	{
		wsprintf(InfoText,"Overiding Section: %s -> Size Of Raw Data from %08X to %08X",sec_iter->Name,sec_iter->SizeOfRawData,sec_iter->Misc.VirtualSize);
		sec_iter->SizeOfRawData			= sec_iter->Misc.VirtualSize;
		OutDebug(hWnd,InfoText);
		wsprintf(InfoText,"Overiding Section: %s -> Pointer To Raw Data from %08X to %08X",sec_iter->Name,sec_iter->PointerToRawData,sec_iter->VirtualAddress);
		sec_iter->PointerToRawData		= sec_iter->VirtualAddress;
		OutDebug(hWnd,InfoText);

		// Fix section characteristics: replace UNINITIALIZED_DATA with CODE if section
		// has EXECUTE permission and contains actual data (not a true BSS section)
		if((sec_iter->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) &&
		   (sec_iter->Characteristics & IMAGE_SCN_MEM_EXECUTE)){
			sec_iter->Characteristics = (sec_iter->Characteristics & ~IMAGE_SCN_CNT_UNINITIALIZED_DATA) | IMAGE_SCN_CNT_CODE;
			wsprintf(InfoText,"Overiding Section: %s -> Fixed characteristics: UNINIT_DATA -> CODE",sec_iter->Name);
			OutDebug(hWnd,InfoText);
		}

		OutDebug(hWnd,"");
		// Next Section
		sec_iter = (IMAGE_SECTION_HEADER *)((UCHAR *)(sec_iter)+sizeof(IMAGE_SECTION_HEADER));
	}

	// =====================================================
	// Phase F2: Detect packer entry point and find OEP
	// =====================================================
	{
		DWORD ep = 0;
		DWORD imageBase32 = 0;
		ULONGLONG imageBase64 = 0;
		if(LoadedPe==TRUE){
			ep = nt_header->OptionalHeader.AddressOfEntryPoint;
			imageBase32 = (DWORD)nt_header->OptionalHeader.ImageBase;
		}
		else if(LoadedPe64==TRUE){
			ep = (DWORD)nt_header64->OptionalHeader.AddressOfEntryPoint;
			imageBase64 = nt_header64->OptionalHeader.ImageBase;
		}

		char *pfile_ep = FileMappedData;
		bool packerEP = false;

		// Check if EP starts with PUSHAD (0x60) - common packer signature
		if(ep > 0 && ep < (DWORD)hFileSize){
			BYTE epByte = (BYTE)pfile_ep[ep];
			if(epByte == 0x60){
				packerEP = true;
				wsprintf(InfoText,"Detected packer entry point at RVA %08X (PUSHAD)", ep);
				OutDebug(hWnd,InfoText);
			}
		}

		if(packerEP){
			// Search for VB5!/VB6! header in the first section
			DWORD searchStart = section_header->VirtualAddress;
			DWORD searchEnd = searchStart + section_header->Misc.VirtualSize;
			if(searchEnd > (DWORD)hFileSize) searchEnd = (DWORD)hFileSize;

			DWORD vbHeaderRVA = 0;
			for(DWORD off = searchStart; off + 4 <= searchEnd; off++){
				if(pfile_ep[off]=='V' && pfile_ep[off+1]=='B' && pfile_ep[off+2]=='5' && pfile_ep[off+3]=='!'){
					vbHeaderRVA = off;
					wsprintf(InfoText,"Found VB5! header at RVA %08X", vbHeaderRVA);
					OutDebug(hWnd,InfoText);
					break;
				}
			}

			if(vbHeaderRVA){
				// Search for PUSH <ImageBase+vbHeaderRVA>; CALL/JMP pattern
				DWORD vbHeaderVA = imageBase32 + vbHeaderRVA;
				BYTE pushPattern[5];
				pushPattern[0] = 0x68; // PUSH imm32
				*(DWORD*)(pushPattern+1) = vbHeaderVA;

				DWORD oep = 0;
				for(DWORD off = searchStart; off + 10 <= searchEnd; off++){
					if(memcmp(pfile_ep + off, pushPattern, 5) == 0){
						BYTE nextByte = (BYTE)pfile_ep[off+5];
						if(nextByte == 0xE8 || nextByte == 0xE9){ // CALL rel32 or JMP rel32
							oep = off;
							break;
						}
						if(nextByte == 0xFF){ // CALL/JMP [mem]
							oep = off;
							break;
						}
					}
				}

				if(oep){
					wsprintf(InfoText,"Found OEP at RVA %08X (PUSH %08X; CALL ThunRTMain)", oep, vbHeaderVA);
					OutDebug(hWnd,InfoText);
					wsprintf(InfoText,"Overiding AddressOfEntryPoint from %08X to %08X", ep, oep);
					OutDebug(hWnd,InfoText);
					if(LoadedPe==TRUE){
						nt_header->OptionalHeader.AddressOfEntryPoint = oep;
					}
					else if(LoadedPe64==TRUE){
						nt_header64->OptionalHeader.AddressOfEntryPoint = oep;
					}
				}
			}
		}
	}

	// =====================================================
	// Phase G: Rebuild the IAT (Import Address Table)
	// =====================================================
	RebuildIAT(hWnd);

	// rebuild task finished
	OutDebug(hWnd,"-> Rebuild Task Completed..");
	OutDebug(hWnd,"====================");
	OutDebug(hWnd,"");
	MessageBox(hWnd,"Rebuilding Task Completed!","PE Repair",MB_OK|MB_ICONINFORMATION);

	// Exit
	return;
}

// ========================================================
// ======= Convert RVA to file offset in a flat PE ========
// ========================================================
static DWORD DllRvaToFileOffset(BYTE *fileBase, DWORD rva)
{
	// Reads section table from the flat PE file and converts
	// an RVA to the corresponding raw file offset.
	LONG lfanew = *(LONG*)(fileBase + 0x3C);
	WORD numSections = *(WORD*)(fileBase + lfanew + 6);
	WORD sizeOfOptHdr = *(WORD*)(fileBase + lfanew + 20);
	BYTE *firstSection = fileBase + lfanew + 24 + sizeOfOptHdr;

	for(WORD i = 0; i < numSections; i++){
		IMAGE_SECTION_HEADER *sec = (IMAGE_SECTION_HEADER*)(firstSection + i * sizeof(IMAGE_SECTION_HEADER));
		if(rva >= sec->VirtualAddress && rva < sec->VirtualAddress + sec->Misc.VirtualSize){
			return rva - sec->VirtualAddress + sec->PointerToRawData;
		}
	}
	return rva; // fallback: assume identity mapping
}

// File-scope struct used by RebuildIAT() and RebuildIAT_ScanUnreferenced()
struct ImportScanEntry {
	DWORD ftRVA;
	int functionCount;
	char dllName[MAX_PATH];
	bool isPackerSuspect;
	int packerScore;
};

// ========================================================
// ==== Raw IAT Reconstruction (destroyed descriptors) ====
// ========================================================
static void RebuildIAT_RawFallback(HWND hWnd)
{
	/*
	   Function:
	   ---------
	   "RebuildIAT_RawFallback"

	   When import descriptors are destroyed (DataDirectory[1] points to
	   garbage/zeroed memory), reconstruct the entire import table from
	   the raw IAT (DataDirectory[12]).

	   The raw IAT contains resolved function addresses grouped by DLL,
	   separated by zero DWORDs. We walk these groups, identify which DLL
	   each belongs to via export table matching, resolve function names,
	   write new import descriptors + hint/name entries to PE header slack,
	   and update DataDirectory[1].

	   PE32 only for initial implementation.
	*/

	char *pfile = FileMappedData;
	char InfoText[512];
	DWORD NumberOfSections = 0;
	DWORD SizeOfImage = 0;

	OutDebug(hWnd,"");
	OutDebug(hWnd,"IAT Rebuild: Import descriptors are destroyed, attempting raw IAT reconstruction...");

	// =====================================================
	// Phase A: Read IAT from DataDirectory[12]
	// =====================================================
	DWORD iatRVA = 0, iatSize = 0;
	if(LoadedPe==TRUE){
		iatRVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
		iatSize = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
		NumberOfSections = nt_header->FileHeader.NumberOfSections;
		SizeOfImage = nt_header->OptionalHeader.SizeOfImage;
	}
	else if(LoadedPe64==TRUE){
		iatRVA = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
		iatSize = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
		NumberOfSections = nt_header64->FileHeader.NumberOfSections;
		SizeOfImage = nt_header64->OptionalHeader.SizeOfImage;
	}

	// SizeOfImage may be corrupted by the packer. Use file size as the reliable
	// threshold for detecting resolved runtime addresses (DLL addresses are always
	// much larger than the PE file size in a memory dump).
	DWORD addrThreshold = (DWORD)hFileSize;
	wsprintf(InfoText,"IAT Rebuild: SizeOfImage=%08X, FileSize=%08X, using threshold=%08X",
		SizeOfImage, (DWORD)hFileSize, addrThreshold);
	OutDebug(hWnd,InfoText);

	// If DataDirectory[12] is also destroyed, scan for IAT at common locations.
	// For VB6/Win32 executables, the IAT is typically at the start of the first section (0x1000).
	if(!iatRVA || !iatSize || iatRVA + iatSize > (DWORD)hFileSize){
		OutDebug(hWnd,"IAT Rebuild: DataDirectory[12] (IAT) is missing or invalid, scanning for IAT...");

		// Get first section's VirtualAddress (after rebuild, file offset == RVA)
		IMAGE_SECTION_HEADER *scanSec;
		if(LoadedPe==TRUE){
			scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
		}
		else{
			scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
		}

		// Scan from the start of each section looking for groups of resolved addresses
		// (values > addrThreshold, meaning they're runtime addresses from a memory dump)
		bool found = false;
		for(DWORD s = 0; s < NumberOfSections && !found; s++){
			DWORD secStart = scanSec->VirtualAddress;
			DWORD secEnd = secStart + scanSec->Misc.VirtualSize;
			if(secEnd > (DWORD)hFileSize) secEnd = (DWORD)hFileSize;

			// Walk the section as DWORDs looking for resolved address groups
			if(secEnd > secStart + sizeof(DWORD) * 4){
				DWORD *data = (DWORD*)(pfile + secStart);
				DWORD numDwords = (secEnd - secStart) / sizeof(DWORD);

				// Look for a sequence of high values (resolved addresses) followed by zero
				int consecutiveHigh = 0;
				DWORD scanStart = 0;
				for(DWORD i = 0; i < numDwords && i < 2048; i++){
					if(data[i] > addrThreshold && data[i] > 0x10000){
						if(consecutiveHigh == 0) scanStart = i;
						consecutiveHigh++;
					}
					else if(data[i] == 0 && consecutiveHigh >= 3){
						// Found a group of resolved addresses followed by zero - this looks like IAT
						iatRVA = secStart + scanStart * sizeof(DWORD);
						// Scan forward to find the extent (until we hit non-IAT data)
						DWORD iatEnd = secStart;
						for(DWORD j = scanStart; j < numDwords && j < 4096; j++){
							if(data[j] == 0){
								// Could be a group separator or end of IAT
								// Check if there's another group after this
								if(j + 1 < numDwords && data[j+1] > addrThreshold && data[j+1] > 0x10000){
									iatEnd = secStart + (j + 1) * sizeof(DWORD);
									continue; // More groups follow
								}
								else{
									iatEnd = secStart + (j + 1) * sizeof(DWORD);
									break;
								}
							}
							iatEnd = secStart + (j + 1) * sizeof(DWORD);
						}
						iatSize = iatEnd - iatRVA;
						found = true;
						break;
					}
					else{
						consecutiveHigh = 0;
					}
				}
			}
			scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(scanSec)+sizeof(IMAGE_SECTION_HEADER));
		}

		if(!found){
			OutDebug(hWnd,"IAT Rebuild: Could not locate IAT data in any section. Reconstruction failed.");
			return;
		}

		wsprintf(InfoText,"IAT Rebuild: Found IAT data at RVA %08X, estimated size %08X", iatRVA, iatSize);
		OutDebug(hWnd,InfoText);
	}
	else{
		wsprintf(InfoText,"IAT Rebuild: Raw IAT at RVA %08X, size %08X", iatRVA, iatSize);
		OutDebug(hWnd,InfoText);
	}

	// Validate final IAT bounds
	if(iatRVA + iatSize > (DWORD)hFileSize){
		iatSize = (DWORD)hFileSize - iatRVA;
	}

	wsprintf(InfoText,"IAT Rebuild: Using IAT at RVA %08X, size %08X", iatRVA, iatSize);
	OutDebug(hWnd,InfoText);

	// =====================================================
	// Phase B: Group IAT entries by DLL
	// =====================================================
	// Walk the IAT as an array of DWORDs (PE32).
	// Non-zero entries are resolved function addresses.
	// A zero entry separates DLL groups.
	struct IATGroup {
		DWORD startRVA;   // RVA of first thunk in this group
		DWORD count;      // number of thunks
	};
	#define MAX_IAT_GROUPS 32
	IATGroup groups[MAX_IAT_GROUPS];
	int numGroups = 0;

	{
		DWORD *iatArray = (DWORD*)(pfile + iatRVA);
		DWORD numEntries = iatSize / sizeof(DWORD);
		DWORD groupStart = 0;
		bool inGroup = false;
		DWORD groupCount = 0;

		for(DWORD i = 0; i < numEntries && numGroups < MAX_IAT_GROUPS; i++){
			DWORD val = iatArray[i];
			// Resolved addresses are > addrThreshold (file size) and > 0x10000
			if(val != 0 && val > addrThreshold && val > 0x10000){
				if(!inGroup){
					groupStart = i;
					groupCount = 0;
					inGroup = true;
				}
				groupCount++;
			}
			else if(val == 0){
				// Zero = group separator
				if(inGroup && groupCount > 0){
					groups[numGroups].startRVA = iatRVA + groupStart * sizeof(DWORD);
					groups[numGroups].count = groupCount;
					numGroups++;
					inGroup = false;
				}
			}
			else{
				// Non-zero but not a resolved address - end of IAT data
				if(inGroup && groupCount > 0){
					groups[numGroups].startRVA = iatRVA + groupStart * sizeof(DWORD);
					groups[numGroups].count = groupCount;
					numGroups++;
				}
				break; // Stop scanning - we've left the IAT area
			}
		}
		// Handle group at end of IAT (no trailing zero)
		if(inGroup && groupCount > 0 && numGroups < MAX_IAT_GROUPS){
			groups[numGroups].startRVA = iatRVA + groupStart * sizeof(DWORD);
			groups[numGroups].count = groupCount;
			numGroups++;
		}
	}

	if(numGroups == 0){
		OutDebug(hWnd,"IAT Rebuild: No IAT groups found in raw IAT.");
		return;
	}

	wsprintf(InfoText,"IAT Rebuild: Found %d IAT groups", numGroups);
	OutDebug(hWnd,InfoText);

	for(int g = 0; g < numGroups; g++){
		wsprintf(InfoText,"IAT Rebuild:   Group %d: RVA=%08X, %d entries", g, groups[g].startRVA, groups[g].count);
		OutDebug(hWnd,InfoText);
	}

	// =====================================================
	// Phase C: Identify DLLs via export table matching
	// =====================================================
	static const char *commonDlls[] = {
		"msvbvm60.dll", "msvbvm50.dll", "kernel32.dll", "user32.dll",
		"gdi32.dll", "advapi32.dll", "ole32.dll", "oleaut32.dll",
		"msvcrt.dll", "ntdll.dll", "kernelbase.dll", "comctl32.dll",
		"shell32.dll", "ws2_32.dll", "comdlg32.dll", "shlwapi.dll",
		"version.dll"
	};
	#define NUM_COMMON_DLLS 17

	// Per-group identification results
	struct GroupInfo {
		char dllName[MAX_PATH];
		DWORD_PTR dllBase;        // runtime base of the DLL
		bool identified;
		// DLL file mapping (kept open for function resolution)
		BYTE *dllFileBase;
		HANDLE hDllMapping;
		HANDLE hDllFile;
		DWORD exportDirRVA;
		DWORD exportDirSize;
		IMAGE_EXPORT_DIRECTORY *exportDir;
		DWORD *functions;
		DWORD *names;
		WORD *ordinals;
	};
	GroupInfo groupInfo[MAX_IAT_GROUPS];
	memset(groupInfo, 0, sizeof(groupInfo));

	for(int g = 0; g < numGroups; g++){
		DWORD *groupAddrs = (DWORD*)(pfile + groups[g].startRVA);

		for(int d = 0; d < NUM_COMMON_DLLS; d++){
			// Open candidate DLL from disk
			HANDLE hDllFile = INVALID_HANDLE_VALUE;
			char dllFilePath[MAX_PATH];

			if(LoadedPe==TRUE){
				char winDir[MAX_PATH];
				GetWindowsDirectoryA(winDir, MAX_PATH);
				wsprintf(dllFilePath, "%s\\SysWOW64\\%s", winDir, commonDlls[d]);
				hDllFile = CreateFileA(dllFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			}
			if(hDllFile == INVALID_HANDLE_VALUE){
				char sysDir[MAX_PATH];
				GetSystemDirectoryA(sysDir, MAX_PATH);
				wsprintf(dllFilePath, "%s\\%s", sysDir, commonDlls[d]);
				hDllFile = CreateFileA(dllFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			}
			if(hDllFile == INVALID_HANDLE_VALUE) continue;

			HANDLE hDllMapping = CreateFileMappingA(hDllFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if(!hDllMapping){ CloseHandle(hDllFile); continue; }
			BYTE *dllFileBase = (BYTE*)MapViewOfFile(hDllMapping, FILE_MAP_READ, 0, 0, 0);
			if(!dllFileBase){ CloseHandle(hDllMapping); CloseHandle(hDllFile); continue; }

			// Validate PE
			if(*(WORD*)dllFileBase != IMAGE_DOS_SIGNATURE){
				UnmapViewOfFile(dllFileBase); CloseHandle(hDllMapping); CloseHandle(hDllFile); continue;
			}
			LONG dllLfanew = *(LONG*)(dllFileBase + 0x3C);
			if(*(DWORD*)(dllFileBase + dllLfanew) != IMAGE_NT_SIGNATURE){
				UnmapViewOfFile(dllFileBase); CloseHandle(hDllMapping); CloseHandle(hDllFile); continue;
			}

			BYTE *dllOptHeader = dllFileBase + dllLfanew + 24;
			WORD dllMagic = *(WORD*)dllOptHeader;
			DWORD expRVA, expSize;
			if(dllMagic == 0x20B){ expRVA = *(DWORD*)(dllOptHeader+112); expSize = *(DWORD*)(dllOptHeader+116); }
			else{ expRVA = *(DWORD*)(dllOptHeader+96); expSize = *(DWORD*)(dllOptHeader+100); }

			if(expRVA == 0 || expSize == 0){
				UnmapViewOfFile(dllFileBase); CloseHandle(hDllMapping); CloseHandle(hDllFile); continue;
			}

			IMAGE_EXPORT_DIRECTORY *expDir = (IMAGE_EXPORT_DIRECTORY*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expRVA));
			DWORD *dllFunctions = (DWORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expDir->AddressOfFunctions));

			// Base detection: test_base = group_addr[0] - export_rva
			// Must be 64KB-aligned
			bool matched = false;
			DWORD_PTR bestBase = 0;
			int bestVerified = 0;

			// Try each export RVA against the first group address
			for(DWORD e = 0; e < expDir->NumberOfFunctions; e++){
				if(dllFunctions[e] == 0) continue;
				// Skip forwarded exports (RVA within export directory range)
				if(dllFunctions[e] >= expRVA && dllFunctions[e] < expRVA + expSize) continue;

				DWORD_PTR testBase = groupAddrs[0] - dllFunctions[e];
				if((testBase & 0xFFFF) != 0) continue; // 64KB-aligned check

				// Verify: count how many group addresses match with this base
				int verified = 0;
				for(DWORD t = 0; t < groups[g].count; t++){
					DWORD testRVA = (DWORD)(groupAddrs[t] - testBase);
					for(DWORD v = 0; v < expDir->NumberOfFunctions; v++){
						if(dllFunctions[v] == testRVA){
							verified++;
							break;
						}
					}
				}

				if(verified > bestVerified){
					bestVerified = verified;
					bestBase = testBase;
				}
				// Early out if we matched all
				if((DWORD)bestVerified == groups[g].count) break;
			}

			// Accept if >= 50% match
			if(bestVerified > 0 && (DWORD)bestVerified >= (groups[g].count + 1) / 2){
				// Keep this DLL mapped for function resolution
				groupInfo[g].identified = true;
				strcpy(groupInfo[g].dllName, commonDlls[d]);
				groupInfo[g].dllBase = bestBase;
				groupInfo[g].dllFileBase = dllFileBase;
				groupInfo[g].hDllMapping = hDllMapping;
				groupInfo[g].hDllFile = hDllFile;
				groupInfo[g].exportDirRVA = expRVA;
				groupInfo[g].exportDirSize = expSize;
				groupInfo[g].exportDir = expDir;
				groupInfo[g].functions = dllFunctions;
				groupInfo[g].names = (DWORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expDir->AddressOfNames));
				groupInfo[g].ordinals = (WORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expDir->AddressOfNameOrdinals));

				wsprintf(InfoText,"IAT Rebuild: Group %d -> %s (base=%08X, matched %d/%d)",
					g, commonDlls[d], (DWORD)bestBase, bestVerified, groups[g].count);
				OutDebug(hWnd,InfoText);
				matched = true;
			}

			if(!matched){
				UnmapViewOfFile(dllFileBase);
				CloseHandle(hDllMapping);
				CloseHandle(hDllFile);
			}
			else{
				break; // Found the DLL for this group
			}
		}

		if(!groupInfo[g].identified){
			wsprintf(InfoText,"IAT Rebuild: Group %d could not be identified", g);
			OutDebug(hWnd,InfoText);
		}
	}

	// Count identified groups
	int identifiedCount = 0;
	for(int g = 0; g < numGroups; g++){
		if(groupInfo[g].identified) identifiedCount++;
	}

	if(identifiedCount == 0){
		OutDebug(hWnd,"IAT Rebuild: No DLLs could be identified from raw IAT. Reconstruction failed.");
		return;
	}

	wsprintf(InfoText,"IAT Rebuild: Identified %d/%d groups", identifiedCount, numGroups);
	OutDebug(hWnd,InfoText);

	// =====================================================
	// Phase D: Find free space in PE header slack
	// =====================================================
	IMAGE_SECTION_HEADER *sec;
	if(LoadedPe==TRUE){
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
	}
	else{
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
	}

	IMAGE_SECTION_HEADER *lastSecEntry = (IMAGE_SECTION_HEADER *)((UCHAR *)(sec) + NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
	DWORD headerSlackStart = (DWORD)((BYTE*)lastSecEntry - (BYTE*)pfile);
	// Align to DWORD boundary
	headerSlackStart = (headerSlackStart + 3) & ~3;

	DWORD firstSectionStart = 0x1000;
	if(sec->VirtualAddress > 0){
		firstSectionStart = sec->VirtualAddress;
	}

	if(firstSectionStart <= headerSlackStart + 64 || firstSectionStart > (DWORD)hFileSize){
		OutDebug(hWnd,"IAT Rebuild: Not enough header slack space for import reconstruction.");
		// Clean up DLL mappings
		for(int g = 0; g < numGroups; g++){
			if(groupInfo[g].identified){
				UnmapViewOfFile(groupInfo[g].dllFileBase);
				CloseHandle(groupInfo[g].hDllMapping);
				CloseHandle(groupInfo[g].hDllFile);
			}
		}
		return;
	}

	DWORD freeSpaceStart = headerSlackStart;
	DWORD freeSpaceEnd = firstSectionStart;
	DWORD freeSpaceSize = freeSpaceEnd - freeSpaceStart;

	wsprintf(InfoText,"IAT Rebuild: Header slack: %d bytes at offset %08X-%08X", freeSpaceSize, freeSpaceStart, freeSpaceEnd);
	OutDebug(hWnd,InfoText);

	// Zero out the header slack area first
	memset(pfile + freeSpaceStart, 0, freeSpaceSize);

	// =====================================================
	// Phase E: Write new import table structure
	// =====================================================
	// Layout:
	//   [IMAGE_IMPORT_DESCRIPTOR #0]   (20 bytes each)
	//   [IMAGE_IMPORT_DESCRIPTOR #1]
	//   ...
	//   [IMAGE_IMPORT_DESCRIPTOR null]  (20 bytes, all zeros - terminator)
	//   "dllname.dll\0"                 (DLL name strings)
	//   [hint=0]["FuncName\0"]          (IMAGE_IMPORT_BY_NAME entries)
	//   ...

	DWORD descriptorStart = freeSpaceStart;
	DWORD descriptorArraySize = (identifiedCount + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR); // +1 for null terminator
	DWORD writeOffset = freeSpaceStart + descriptorArraySize;
	int totalResolved = 0;
	int totalUnresolved = 0;
	int descIndex = 0;

	IMAGE_IMPORT_DESCRIPTOR *newDescs = (IMAGE_IMPORT_DESCRIPTOR*)(pfile + descriptorStart);

	for(int g = 0; g < numGroups; g++){
		if(!groupInfo[g].identified) continue;

		// Check space for DLL name
		DWORD dllNameLen = 0;
		while(groupInfo[g].dllName[dllNameLen] != 0) dllNameLen++;

		if(writeOffset + dllNameLen + 1 >= freeSpaceEnd){
			wsprintf(InfoText,"IAT Rebuild: Out of header space writing DLL name for group %d", g);
			OutDebug(hWnd,InfoText);
			break;
		}

		// Write DLL name string
		DWORD dllNameRVA = writeOffset; // In header area, file offset == RVA
		for(DWORD c = 0; c <= dllNameLen; c++){
			pfile[writeOffset + c] = groupInfo[g].dllName[c];
		}
		writeOffset += dllNameLen + 1;
		// WORD-align
		writeOffset = (writeOffset + 1) & ~1;

		// Fill in the import descriptor
		newDescs[descIndex].OriginalFirstThunk = 0; // no OFT
		newDescs[descIndex].TimeDateStamp = 0;
		newDescs[descIndex].ForwarderChain = 0;
		newDescs[descIndex].Name = dllNameRVA;
		newDescs[descIndex].FirstThunk = groups[g].startRVA;

		wsprintf(InfoText,"IAT Rebuild: Writing descriptor %d: %s, FirstThunk=%08X",
			descIndex, groupInfo[g].dllName, groups[g].startRVA);
		OutDebug(hWnd,InfoText);

		// Resolve each function in this group and write hint/name entries
		DWORD *groupAddrs = (DWORD*)(pfile + groups[g].startRVA);
		for(DWORD t = 0; t < groups[g].count; t++){
			DWORD addr = groupAddrs[t];
			DWORD expRVA = (DWORD)(addr - groupInfo[g].dllBase);
			char *funcName = NULL;

			// Find the function name by matching the export RVA
			for(DWORD e = 0; e < groupInfo[g].exportDir->NumberOfFunctions; e++){
				if(groupInfo[g].functions[e] == expRVA){
					for(DWORD n = 0; n < groupInfo[g].exportDir->NumberOfNames; n++){
						if(groupInfo[g].ordinals[n] == e){
							funcName = (char*)(groupInfo[g].dllFileBase + DllRvaToFileOffset(groupInfo[g].dllFileBase, groupInfo[g].names[n]));
							break;
						}
					}
					break;
				}
			}

			if(funcName != NULL){
				DWORD nameLen = 0;
				while(funcName[nameLen] != 0) nameLen++;
				DWORD entrySize = sizeof(WORD) + nameLen + 1;
				entrySize = (entrySize + 1) & ~1; // WORD-align

				if(writeOffset + entrySize < freeSpaceEnd){
					// Write IMAGE_IMPORT_BY_NAME: hint(WORD) + name string
					*(WORD*)(pfile + writeOffset) = 0; // hint = 0
					for(DWORD c = 0; c <= nameLen; c++){
						pfile[writeOffset + sizeof(WORD) + c] = funcName[c];
					}
					// Overwrite the IAT entry with the RVA of the hint/name entry
					groupAddrs[t] = writeOffset;
					wsprintf(InfoText,"  %s!%s", groupInfo[g].dllName, funcName);
					OutDebug(hWnd,InfoText);
					totalResolved++;
					writeOffset += entrySize;
				}
			}
			else{
				// Could not resolve - write placeholder
				char placeholder[64];
				wsprintf(placeholder, "Unresolved_%08X", addr);
				DWORD nameLen = 0;
				while(placeholder[nameLen] != 0) nameLen++;
				DWORD entrySize = sizeof(WORD) + nameLen + 1;
				entrySize = (entrySize + 1) & ~1;

				if(writeOffset + entrySize < freeSpaceEnd){
					*(WORD*)(pfile + writeOffset) = 0;
					for(DWORD c = 0; c <= nameLen; c++){
						pfile[writeOffset + sizeof(WORD) + c] = placeholder[c];
					}
					groupAddrs[t] = writeOffset;
					wsprintf(InfoText,"  %s!%s (unresolved)", groupInfo[g].dllName, placeholder);
					OutDebug(hWnd,InfoText);
					writeOffset += entrySize;
				}
				totalUnresolved++;
			}
		}

		descIndex++;
	}

	// Null-terminate the descriptor array (already zeroed by memset)

	// =====================================================
	// Phase F: Update DataDirectory[1] (Import Directory)
	// =====================================================
	if(LoadedPe==TRUE){
		nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = descriptorStart;
		nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = descriptorArraySize;
	}
	else if(LoadedPe64==TRUE){
		nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = descriptorStart;
		nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = descriptorArraySize;
	}

	wsprintf(InfoText,"IAT Rebuild: Updated DataDirectory[1]: RVA=%08X, Size=%08X", descriptorStart, descriptorArraySize);
	OutDebug(hWnd,InfoText);

	// =====================================================
	// Phase G: Log results and clean up
	// =====================================================
	OutDebug(hWnd,"");
	wsprintf(InfoText,"IAT Rebuild: Raw IAT reconstruction complete");
	OutDebug(hWnd,InfoText);
	wsprintf(InfoText,"IAT Rebuild: %d groups found, %d DLLs identified", numGroups, identifiedCount);
	OutDebug(hWnd,InfoText);
	wsprintf(InfoText,"IAT Rebuild: %d functions resolved, %d unresolved", totalResolved, totalUnresolved);
	OutDebug(hWnd,InfoText);
	wsprintf(InfoText,"IAT Rebuild: Used %d/%d bytes of header slack", writeOffset - freeSpaceStart, freeSpaceSize);
	OutDebug(hWnd,InfoText);

	// Clean up DLL mappings
	for(int g = 0; g < numGroups; g++){
		if(groupInfo[g].identified){
			UnmapViewOfFile(groupInfo[g].dllFileBase);
			CloseHandle(groupInfo[g].hDllMapping);
			CloseHandle(groupInfo[g].hDllFile);
		}
	}
}

// ========================================================
// ==== Scan for unreferenced IAT groups (Scylla-style) ===
// ========================================================
static void RebuildIAT_ScanUnreferenced(
	HWND hWnd,
	DWORD *pWriteOffset,
	DWORD spaceEnd,
	ImportScanEntry *importScan,
	int importScanCount,
	PIMAGE_IMPORT_DESCRIPTOR importDesc)
{
	/*
	   When ALL import descriptors are packer-suspect, the real IAT may exist
	   elsewhere in the PE but no descriptor references it. This function scans
	   all sections for contiguous groups of resolved addresses (Scylla-style),
	   filters out groups already referenced by packer descriptors, identifies
	   DLLs via export table matching, and writes new import descriptors +
	   hint/name entries into header slack space.
	*/

	char *pfile = FileMappedData;
	char InfoText[512];
	DWORD NumberOfSections = 0;
	DWORD SizeOfImage = 0;
	DWORD SizeOfHeaders = 0;

	if(LoadedPe==TRUE){
		NumberOfSections = nt_header->FileHeader.NumberOfSections;
		SizeOfImage = nt_header->OptionalHeader.SizeOfImage;
		SizeOfHeaders = nt_header->OptionalHeader.SizeOfHeaders;
	}
	else if(LoadedPe64==TRUE){
		NumberOfSections = nt_header64->FileHeader.NumberOfSections;
		SizeOfImage = nt_header64->OptionalHeader.SizeOfImage;
		SizeOfHeaders = nt_header64->OptionalHeader.SizeOfHeaders;
	}

	OutDebug(hWnd,"");
	OutDebug(hWnd,"IAT Rebuild Phase 6: Scanning all sections for unreferenced IAT groups...");

	// =====================================================
	// Phase S-A: Scan sections for resolved address groups
	// =====================================================
	struct IATGroup {
		DWORD startRVA;
		DWORD count;
	};
	#define MAX_SCAN_GROUPS 32
	IATGroup groups[MAX_SCAN_GROUPS];
	int numGroups = 0;

	IMAGE_SECTION_HEADER *sec;
	if(LoadedPe==TRUE){
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
	}
	else{
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
	}

	IMAGE_SECTION_HEADER *scanSec = sec;
	for(DWORD s = 0; s < NumberOfSections && numGroups < MAX_SCAN_GROUPS; s++){
		DWORD secStart = scanSec->VirtualAddress;
		DWORD secEnd = secStart + scanSec->Misc.VirtualSize;
		if(secEnd > (DWORD)hFileSize) secEnd = (DWORD)hFileSize;
		if(secStart >= (DWORD)hFileSize || secEnd <= secStart){
			scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(scanSec)+sizeof(IMAGE_SECTION_HEADER));
			continue;
		}

		if(LoadedPe==TRUE){
			// PE32: scan as DWORD array
			DWORD *data = (DWORD*)(pfile + secStart);
			DWORD numDwords = (secEnd - secStart) / sizeof(DWORD);
			DWORD groupStart = 0;
			DWORD groupCount = 0;
			bool inGroup = false;

			for(DWORD i = 0; i < numDwords && numGroups < MAX_SCAN_GROUPS; i++){
				DWORD val = data[i];
				if(val > SizeOfImage && val > 0x10000){
					if(!inGroup){
						groupStart = i;
						groupCount = 0;
						inGroup = true;
					}
					groupCount++;
				}
				else{
					if(inGroup && groupCount >= 2){
						DWORD grpRVA = secStart + groupStart * sizeof(DWORD);
						if(grpRVA >= SizeOfHeaders){
							groups[numGroups].startRVA = grpRVA;
							groups[numGroups].count = groupCount;
							numGroups++;
						}
					}
					inGroup = false;
					groupCount = 0;
				}
			}
			// Handle group at end of section
			if(inGroup && groupCount >= 2 && numGroups < MAX_SCAN_GROUPS){
				DWORD grpRVA = secStart + groupStart * sizeof(DWORD);
				if(grpRVA >= SizeOfHeaders){
					groups[numGroups].startRVA = grpRVA;
					groups[numGroups].count = groupCount;
					numGroups++;
				}
			}
		}
		else if(LoadedPe64==TRUE){
			// PE64: scan as QWORD array
			ULONGLONG *data = (ULONGLONG*)(pfile + secStart);
			DWORD numQwords = (secEnd - secStart) / sizeof(ULONGLONG);
			DWORD groupStart = 0;
			DWORD groupCount = 0;
			bool inGroup = false;

			for(DWORD i = 0; i < numQwords && numGroups < MAX_SCAN_GROUPS; i++){
				ULONGLONG val = data[i];
				if(val > SizeOfImage && val > 0x10000){
					if(!inGroup){
						groupStart = i;
						groupCount = 0;
						inGroup = true;
					}
					groupCount++;
				}
				else{
					if(inGroup && groupCount >= 2){
						DWORD grpRVA = secStart + groupStart * sizeof(ULONGLONG);
						if(grpRVA >= SizeOfHeaders){
							groups[numGroups].startRVA = grpRVA;
							groups[numGroups].count = groupCount;
							numGroups++;
						}
					}
					inGroup = false;
					groupCount = 0;
				}
			}
			if(inGroup && groupCount >= 2 && numGroups < MAX_SCAN_GROUPS){
				DWORD grpRVA = secStart + groupStart * sizeof(ULONGLONG);
				if(grpRVA >= SizeOfHeaders){
					groups[numGroups].startRVA = grpRVA;
					groups[numGroups].count = groupCount;
					numGroups++;
				}
			}
		}

		scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(scanSec)+sizeof(IMAGE_SECTION_HEADER));
	}

	wsprintf(InfoText,"IAT Rebuild Phase 6: Found %d candidate IAT groups", numGroups);
	OutDebug(hWnd,InfoText);

	if(numGroups == 0){
		OutDebug(hWnd,"IAT Rebuild Phase 6: No unreferenced IAT groups found.");
		return;
	}

	for(int g = 0; g < numGroups; g++){
		wsprintf(InfoText,"IAT Rebuild Phase 6:   Group %d: RVA=%08X, %d entries", g, groups[g].startRVA, groups[g].count);
		OutDebug(hWnd,InfoText);
	}

	// =====================================================
	// Phase S-B: Filter out packer-referenced groups
	// =====================================================
	for(int g = 0; g < numGroups; g++){
		for(int i = 0; i < importScanCount; i++){
			if(groups[g].startRVA == importScan[i].ftRVA){
				wsprintf(InfoText,"IAT Rebuild Phase 6:   Group %d at RVA %08X already referenced by packer descriptor, removing",
					g, groups[g].startRVA);
				OutDebug(hWnd,InfoText);
				// Remove by shifting remaining groups down
				for(int j = g; j < numGroups - 1; j++){
					groups[j] = groups[j+1];
				}
				numGroups--;
				g--; // re-check this index
				break;
			}
		}
	}

	if(numGroups == 0){
		OutDebug(hWnd,"IAT Rebuild Phase 6: All groups are already referenced by packer descriptors.");
		return;
	}

	wsprintf(InfoText,"IAT Rebuild Phase 6: %d unreferenced groups remain after filtering", numGroups);
	OutDebug(hWnd,InfoText);

	// =====================================================
	// Phase S-C: Identify DLLs via export table matching
	// =====================================================
	static const char *commonDlls[] = {
		"msvbvm60.dll", "msvbvm50.dll", "kernel32.dll", "user32.dll",
		"gdi32.dll", "advapi32.dll", "ole32.dll", "oleaut32.dll",
		"msvcrt.dll", "ntdll.dll", "kernelbase.dll", "comctl32.dll",
		"shell32.dll", "ws2_32.dll", "comdlg32.dll", "shlwapi.dll",
		"version.dll"
	};
	#define NUM_SCAN_DLLS 17

	struct ScanGroupInfo {
		char dllName[MAX_PATH];
		DWORD_PTR dllBase;
		bool identified;
		BYTE *dllFileBase;
		HANDLE hDllMapping;
		HANDLE hDllFile;
		DWORD exportDirRVA;
		DWORD exportDirSize;
		IMAGE_EXPORT_DIRECTORY *exportDir;
		DWORD *functions;
		DWORD *names;
		WORD *ordinals;
	};
	ScanGroupInfo groupInfo[MAX_SCAN_GROUPS];
	memset(groupInfo, 0, sizeof(groupInfo));

	for(int g = 0; g < numGroups; g++){
		for(int d = 0; d < NUM_SCAN_DLLS; d++){
			HANDLE hDllFile = INVALID_HANDLE_VALUE;
			char dllFilePath[MAX_PATH];

			if(LoadedPe==TRUE){
				char winDir[MAX_PATH];
				GetWindowsDirectoryA(winDir, MAX_PATH);
				wsprintf(dllFilePath, "%s\\SysWOW64\\%s", winDir, commonDlls[d]);
				hDllFile = CreateFileA(dllFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			}
			if(hDllFile == INVALID_HANDLE_VALUE){
				char sysDir[MAX_PATH];
				GetSystemDirectoryA(sysDir, MAX_PATH);
				wsprintf(dllFilePath, "%s\\%s", sysDir, commonDlls[d]);
				hDllFile = CreateFileA(dllFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			}
			if(hDllFile == INVALID_HANDLE_VALUE) continue;

			HANDLE hDllMapping = CreateFileMappingA(hDllFile, NULL, PAGE_READONLY, 0, 0, NULL);
			if(!hDllMapping){ CloseHandle(hDllFile); continue; }
			BYTE *dllFileBase = (BYTE*)MapViewOfFile(hDllMapping, FILE_MAP_READ, 0, 0, 0);
			if(!dllFileBase){ CloseHandle(hDllMapping); CloseHandle(hDllFile); continue; }

			// Validate PE
			if(*(WORD*)dllFileBase != IMAGE_DOS_SIGNATURE){
				UnmapViewOfFile(dllFileBase); CloseHandle(hDllMapping); CloseHandle(hDllFile); continue;
			}
			LONG dllLfanew = *(LONG*)(dllFileBase + 0x3C);
			if(*(DWORD*)(dllFileBase + dllLfanew) != IMAGE_NT_SIGNATURE){
				UnmapViewOfFile(dllFileBase); CloseHandle(hDllMapping); CloseHandle(hDllFile); continue;
			}

			BYTE *dllOptHeader = dllFileBase + dllLfanew + 24;
			WORD dllMagic = *(WORD*)dllOptHeader;
			DWORD expRVA, expSize;
			if(dllMagic == 0x20B){ expRVA = *(DWORD*)(dllOptHeader+112); expSize = *(DWORD*)(dllOptHeader+116); }
			else{ expRVA = *(DWORD*)(dllOptHeader+96); expSize = *(DWORD*)(dllOptHeader+100); }

			if(expRVA == 0 || expSize == 0){
				UnmapViewOfFile(dllFileBase); CloseHandle(hDllMapping); CloseHandle(hDllFile); continue;
			}

			IMAGE_EXPORT_DIRECTORY *expDir = (IMAGE_EXPORT_DIRECTORY*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expRVA));
			DWORD *dllFunctions = (DWORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expDir->AddressOfFunctions));

			// Base detection: try each non-forwarded export against first group entry
			bool matched = false;
			DWORD_PTR bestBase = 0;
			int bestVerified = 0;

			if(LoadedPe==TRUE){
				DWORD *groupAddrs = (DWORD*)(pfile + groups[g].startRVA);
				for(DWORD e = 0; e < expDir->NumberOfFunctions; e++){
					if(dllFunctions[e] == 0) continue;
					if(dllFunctions[e] >= expRVA && dllFunctions[e] < expRVA + expSize) continue;

					DWORD_PTR testBase = groupAddrs[0] - dllFunctions[e];
					if((testBase & 0xFFFF) != 0) continue;

					int verified = 0;
					for(DWORD t = 0; t < groups[g].count; t++){
						DWORD testRVA = (DWORD)(groupAddrs[t] - testBase);
						for(DWORD v = 0; v < expDir->NumberOfFunctions; v++){
							if(dllFunctions[v] == testRVA){
								verified++;
								break;
							}
						}
					}

					if(verified > bestVerified){
						bestVerified = verified;
						bestBase = testBase;
					}
					if((DWORD)bestVerified == groups[g].count) break;
				}
			}
			else if(LoadedPe64==TRUE){
				ULONGLONG *groupAddrs = (ULONGLONG*)(pfile + groups[g].startRVA);
				for(DWORD e = 0; e < expDir->NumberOfFunctions; e++){
					if(dllFunctions[e] == 0) continue;
					if(dllFunctions[e] >= expRVA && dllFunctions[e] < expRVA + expSize) continue;

					ULONGLONG testBase = groupAddrs[0] - dllFunctions[e];
					if((testBase & 0xFFFF) != 0) continue;

					int verified = 0;
					for(DWORD t = 0; t < groups[g].count; t++){
						DWORD testRVA = (DWORD)(groupAddrs[t] - testBase);
						for(DWORD v = 0; v < expDir->NumberOfFunctions; v++){
							if(dllFunctions[v] == testRVA){
								verified++;
								break;
							}
						}
					}

					if(verified > bestVerified){
						bestVerified = verified;
						bestBase = (DWORD_PTR)testBase;
					}
					if((DWORD)bestVerified == groups[g].count) break;
				}
			}

			// Require at least 3 verified matches to avoid false positives from small groups
			if(bestVerified >= 3 && (DWORD)bestVerified >= (groups[g].count + 1) / 2){
				groupInfo[g].identified = true;
				strcpy(groupInfo[g].dllName, commonDlls[d]);
				groupInfo[g].dllBase = bestBase;
				groupInfo[g].dllFileBase = dllFileBase;
				groupInfo[g].hDllMapping = hDllMapping;
				groupInfo[g].hDllFile = hDllFile;
				groupInfo[g].exportDirRVA = expRVA;
				groupInfo[g].exportDirSize = expSize;
				groupInfo[g].exportDir = expDir;
				groupInfo[g].functions = dllFunctions;
				groupInfo[g].names = (DWORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expDir->AddressOfNames));
				groupInfo[g].ordinals = (WORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, expDir->AddressOfNameOrdinals));

				wsprintf(InfoText,"IAT Rebuild Phase 6: Group %d -> %s (base=%08X, matched %d/%d)",
					g, commonDlls[d], (DWORD)bestBase, bestVerified, groups[g].count);
				OutDebug(hWnd,InfoText);
				matched = true;
			}

			if(!matched){
				UnmapViewOfFile(dllFileBase);
				CloseHandle(hDllMapping);
				CloseHandle(hDllFile);
			}
			else{
				break;
			}
		}

		if(!groupInfo[g].identified){
			wsprintf(InfoText,"IAT Rebuild Phase 6: Group %d could not be identified", g);
			OutDebug(hWnd,InfoText);
		}
	}

	int identifiedCount = 0;
	for(int g = 0; g < numGroups; g++){
		if(groupInfo[g].identified) identifiedCount++;
	}

	if(identifiedCount == 0){
		OutDebug(hWnd,"IAT Rebuild Phase 6: No DLLs could be identified. Scanner failed.");
		return;
	}

	wsprintf(InfoText,"IAT Rebuild Phase 6: Identified %d/%d groups", identifiedCount, numGroups);
	OutDebug(hWnd,InfoText);

	// =====================================================
	// Phase S-D: Build new import table in header slack
	// =====================================================
	DWORD writeOffset = *pWriteOffset;

	// Layout: descriptors first, then DLL names and hint/name entries
	DWORD descriptorArraySize = (identifiedCount + 1) * sizeof(IMAGE_IMPORT_DESCRIPTOR);

	if(writeOffset + descriptorArraySize >= spaceEnd){
		OutDebug(hWnd,"IAT Rebuild Phase 6: Not enough space for import descriptors.");
		goto cleanup;
	}

	{
		DWORD descriptorStart = writeOffset;
		// Zero out the descriptor area
		memset(pfile + descriptorStart, 0, descriptorArraySize);
		IMAGE_IMPORT_DESCRIPTOR *newDescs = (IMAGE_IMPORT_DESCRIPTOR*)(pfile + descriptorStart);
		writeOffset += descriptorArraySize;

		int totalResolved = 0;
		int totalUnresolved = 0;
		int descIndex = 0;

		for(int g = 0; g < numGroups; g++){
			if(!groupInfo[g].identified) continue;

			// Write DLL name string
			DWORD dllNameLen = (DWORD)strlen(groupInfo[g].dllName);
			if(writeOffset + dllNameLen + 1 >= spaceEnd){
				wsprintf(InfoText,"IAT Rebuild Phase 6: Out of space writing DLL name for group %d", g);
				OutDebug(hWnd,InfoText);
				break;
			}

			DWORD dllNameRVA = writeOffset;
			memcpy(pfile + writeOffset, groupInfo[g].dllName, dllNameLen + 1);
			writeOffset += dllNameLen + 1;
			writeOffset = (writeOffset + 1) & ~1; // WORD-align

			// Fill in the import descriptor
			newDescs[descIndex].OriginalFirstThunk = 0;
			newDescs[descIndex].TimeDateStamp = 0;
			newDescs[descIndex].ForwarderChain = 0;
			newDescs[descIndex].Name = dllNameRVA;
			newDescs[descIndex].FirstThunk = groups[g].startRVA;

			wsprintf(InfoText,"IAT Rebuild Phase 6: Descriptor %d: %s, FirstThunk=%08X",
				descIndex, groupInfo[g].dllName, groups[g].startRVA);
			OutDebug(hWnd,InfoText);

			// Resolve each function in this group and write hint/name entries
			for(DWORD t = 0; t < groups[g].count; t++){
				DWORD addr;
				if(LoadedPe==TRUE){
					addr = *(DWORD*)(pfile + groups[g].startRVA + t * sizeof(DWORD));
				}
				else{
					addr = (DWORD)(*(ULONGLONG*)(pfile + groups[g].startRVA + t * sizeof(ULONGLONG)) - groupInfo[g].dllBase);
					// For PE64, expRVA is already the offset we need
					// Re-read the raw value for export matching
					ULONGLONG rawVal = *(ULONGLONG*)(pfile + groups[g].startRVA + t * sizeof(ULONGLONG));
					addr = (DWORD)(rawVal - groupInfo[g].dllBase);
				}

				DWORD expRVA_func;
				if(LoadedPe==TRUE){
					expRVA_func = (DWORD)(addr - groupInfo[g].dllBase);
				}
				else{
					expRVA_func = addr; // already computed above for PE64
				}

				char *funcName = NULL;
				for(DWORD e = 0; e < groupInfo[g].exportDir->NumberOfFunctions; e++){
					if(groupInfo[g].functions[e] == expRVA_func){
						for(DWORD n = 0; n < groupInfo[g].exportDir->NumberOfNames; n++){
							if(groupInfo[g].ordinals[n] == e){
								funcName = (char*)(groupInfo[g].dllFileBase + DllRvaToFileOffset(groupInfo[g].dllFileBase, groupInfo[g].names[n]));
								break;
							}
						}
						break;
					}
				}

				if(funcName != NULL){
					DWORD nameLen = (DWORD)strlen(funcName);
					DWORD entrySize = sizeof(WORD) + nameLen + 1;
					entrySize = (entrySize + 1) & ~1; // WORD-align

					if(writeOffset + entrySize < spaceEnd){
						*(WORD*)(pfile + writeOffset) = 0; // hint = 0
						memcpy(pfile + writeOffset + sizeof(WORD), funcName, nameLen + 1);

						// Overwrite the IAT thunk with RVA of hint/name entry
						if(LoadedPe==TRUE){
							*(DWORD*)(pfile + groups[g].startRVA + t * sizeof(DWORD)) = writeOffset;
						}
						else{
							*(ULONGLONG*)(pfile + groups[g].startRVA + t * sizeof(ULONGLONG)) = writeOffset;
						}

						wsprintf(InfoText,"  %s!%s", groupInfo[g].dllName, funcName);
						OutDebug(hWnd,InfoText);
						totalResolved++;
						writeOffset += entrySize;
					}
				}
				else{
					// Zero the thunk - don't write a fake name the loader can't resolve
					if(LoadedPe==TRUE){
						*(DWORD*)(pfile + groups[g].startRVA + t * sizeof(DWORD)) = 0;
					}
					else{
						*(ULONGLONG*)(pfile + groups[g].startRVA + t * sizeof(ULONGLONG)) = 0;
					}
					totalUnresolved++;
				}
			}

			// Write a zero terminator after the last thunk so ShowImports/loader stops
			if(LoadedPe==TRUE){
				DWORD termOff = groups[g].startRVA + groups[g].count * sizeof(DWORD);
				if(termOff + sizeof(DWORD) <= (DWORD)hFileSize){
					*(DWORD*)(pfile + termOff) = 0;
				}
			}
			else{
				DWORD termOff = groups[g].startRVA + groups[g].count * sizeof(ULONGLONG);
				if(termOff + sizeof(ULONGLONG) <= (DWORD)hFileSize){
					*(ULONGLONG*)(pfile + termOff) = 0;
				}
			}

			descIndex++;
		}

		// =====================================================
		// Phase S-E: Update DataDirectory and zero packer descriptors
		// =====================================================
		if(LoadedPe==TRUE){
			nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = descriptorStart;
			nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = descriptorArraySize;
		}
		else if(LoadedPe64==TRUE){
			nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = descriptorStart;
			nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = descriptorArraySize;
		}

		wsprintf(InfoText,"IAT Rebuild Phase 6: Updated DataDirectory[1]: RVA=%08X, Size=%08X", descriptorStart, descriptorArraySize);
		OutDebug(hWnd,InfoText);

		// Update DataDirectory[12] (IAT) to cover all identified group thunk ranges
		{
			DWORD iatStart = 0xFFFFFFFF, iatEnd = 0;
			DWORD thunkSize = LoadedPe ? sizeof(DWORD) : sizeof(ULONGLONG);
			for(int g2 = 0; g2 < numGroups; g2++){
				if(!groupInfo[g2].identified) continue;
				if(groups[g2].startRVA < iatStart) iatStart = groups[g2].startRVA;
				DWORD grpEnd = groups[g2].startRVA + (groups[g2].count + 1) * thunkSize; // +1 for null terminator
				if(grpEnd > iatEnd) iatEnd = grpEnd;
			}
			if(iatStart < iatEnd){
				if(LoadedPe==TRUE){
					nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = iatStart;
					nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = iatEnd - iatStart;
				}
				else if(LoadedPe64==TRUE){
					nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = iatStart;
					nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = iatEnd - iatStart;
				}
				wsprintf(InfoText,"IAT Rebuild Phase 6: Updated DataDirectory[12] (IAT): RVA=%08X, Size=%08X", iatStart, iatEnd - iatStart);
				OutDebug(hWnd,InfoText);
			}
		}

		// Zero out ALL old packer descriptors so the loader ignores them
		{
			PIMAGE_IMPORT_DESCRIPTOR zeroDesc = importDesc;
			int zeroed = 0;
			while((BYTE*)zeroDesc < (BYTE*)pfile + hFileSize - sizeof(IMAGE_IMPORT_DESCRIPTOR)){
				if(zeroDesc->TimeDateStamp == 0 && zeroDesc->Name == 0)
					break;
				zeroDesc->Name = 0;
				zeroDesc->FirstThunk = 0;
				zeroDesc->OriginalFirstThunk = 0;
				zeroDesc->TimeDateStamp = 0;
				zeroDesc->ForwarderChain = 0;
				zeroed++;
				zeroDesc++;
			}
			wsprintf(InfoText,"IAT Rebuild Phase 6: Zeroed %d old packer descriptors", zeroed);
			OutDebug(hWnd,InfoText);
		}

		*pWriteOffset = writeOffset;

		OutDebug(hWnd,"");
		wsprintf(InfoText,"IAT Rebuild Phase 6: Scanner complete - %d groups, %d identified", numGroups, identifiedCount);
		OutDebug(hWnd,InfoText);
		wsprintf(InfoText,"IAT Rebuild Phase 6: %d functions resolved, %d unresolved", totalResolved, totalUnresolved);
		OutDebug(hWnd,InfoText);
	}

	// =====================================================
	// Phase S-F: Cleanup
	// =====================================================
cleanup:
	for(int g = 0; g < numGroups; g++){
		if(groupInfo[g].identified){
			UnmapViewOfFile(groupInfo[g].dllFileBase);
			CloseHandle(groupInfo[g].hDllMapping);
			CloseHandle(groupInfo[g].hDllFile);
		}
	}
}

// ========================================================
// ============ Rebuild IAT (Import Address Table) ========
// ========================================================
void RebuildIAT(HWND hWnd)
{
	/*
	   Function:
	   ---------
	   "RebuildIAT"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler for debug output

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   This function detects if the IAT contains resolved runtime
	   addresses (from a memory dump) and rebuilds it by resolving
	   function names from the actual DLL export tables.
	   After section rebuild, file offset == RVA, so delta = 0.
	*/

	PIMAGE_IMPORT_DESCRIPTOR importDesc;
	IMAGE_SECTION_HEADER *sec;
	DWORD importsStartRVA = 0;
	DWORD NumberOfSections = 0;
	DWORD SizeOfImage = 0;
	char *pfile = FileMappedData;
	char InfoText[512];

	OutDebug(hWnd,"");
	OutDebug(hWnd,"IAT Rebuild: Checking imports...");

	// After section rebuild, PointerToRawData == VirtualAddress, so delta = 0
	// This means file_offset == RVA for all data

	// Get import directory RVA
	if(LoadedPe==TRUE){
		importsStartRVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		NumberOfSections = nt_header->FileHeader.NumberOfSections;
		SizeOfImage = nt_header->OptionalHeader.SizeOfImage;
	}
	else if(LoadedPe64==TRUE){
		importsStartRVA = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		NumberOfSections = nt_header64->FileHeader.NumberOfSections;
		SizeOfImage = nt_header64->OptionalHeader.SizeOfImage;
	}

	if(!importsStartRVA){
		OutDebug(hWnd,"IAT Rebuild: No import directory found.");
		RebuildIAT_RawFallback(hWnd);
		return;
	}

	// Bounds check: import directory must be within the file
	if(importsStartRVA >= (DWORD)hFileSize){
		OutDebug(hWnd,"IAT Rebuild: Import directory RVA is outside the file.");
		RebuildIAT_RawFallback(hWnd);
		return;
	}

	// Since after rebuild delta=0, file offset == RVA
	importDesc = (PIMAGE_IMPORT_DESCRIPTOR)(importsStartRVA + (DWORD)pfile);

	// Validate pointer is within mapped file
	if((BYTE*)importDesc < (BYTE*)pfile || (BYTE*)importDesc >= (BYTE*)pfile + hFileSize){
		OutDebug(hWnd,"IAT Rebuild: Import descriptor pointer out of bounds.");
		RebuildIAT_RawFallback(hWnd);
		return;
	}

	// =====================================================
	// Phase 1: Detect if IAT needs rebuilding
	// =====================================================
	// Walk all import descriptors and check if thunk values look like resolved addresses
	bool needsRebuild = false;
	int totalThunks = 0;
	int resolvedThunks = 0;

	{
		PIMAGE_IMPORT_DESCRIPTOR scanDesc = importDesc;
		while((BYTE*)scanDesc < (BYTE*)pfile + hFileSize - sizeof(IMAGE_IMPORT_DESCRIPTOR)){
			if(scanDesc->TimeDateStamp == 0 && scanDesc->Name == 0)
				break;
			if(scanDesc->Name == 0 || scanDesc->Name >= (DWORD)hFileSize)
				break;

			DWORD ftRVA = scanDesc->FirstThunk;
			if(ftRVA == 0 || ftRVA >= (DWORD)hFileSize){
				scanDesc++;
				continue;
			}

			if(LoadedPe==TRUE){
				PIMAGE_THUNK_DATA32 scanThunk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
				for(int t=0; t<2000; t++){
					if((BYTE*)scanThunk >= (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32))
						break;
					if(scanThunk->u1.AddressOfData == 0)
						break;
					totalThunks++;
					DWORD val = (DWORD)scanThunk->u1.AddressOfData;
					// If the thunk value is > 0x10000 and outside the PE's virtual range,
					// it's likely a resolved runtime address
					if(val > 0x10000 && val > SizeOfImage){
						resolvedThunks++;
					}
					scanThunk++;
				}
			}
			else if(LoadedPe64==TRUE){
				PIMAGE_THUNK_DATA64 scanThunk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
				for(int t=0; t<2000; t++){
					if((BYTE*)scanThunk64 >= (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64))
						break;
					if(scanThunk64->u1.AddressOfData == 0)
						break;
					totalThunks++;
					ULONGLONG val = scanThunk64->u1.AddressOfData;
					if(val > 0x10000 && val > SizeOfImage){
						resolvedThunks++;
					}
					scanThunk64++;
				}
			}
			scanDesc++;
		}
	}

	if(totalThunks == 0){
		OutDebug(hWnd,"IAT Rebuild: No thunk entries found in import descriptors.");
		RebuildIAT_RawFallback(hWnd);
		return;
	}

	// If less than half the thunks look resolved, IAT is probably fine
	if(resolvedThunks < totalThunks / 2){
		wsprintf(InfoText,"IAT Rebuild: IAT appears intact (%d/%d thunks valid). Skipping.", totalThunks - resolvedThunks, totalThunks);
		OutDebug(hWnd,InfoText);
		return;
	}

	wsprintf(InfoText,"IAT Rebuild: Detected %d/%d resolved thunks. Rebuilding...", resolvedThunks, totalThunks);
	OutDebug(hWnd,InfoText);

	// =====================================================
	// Phase 1b: Packer import detection
	// =====================================================
	// Pre-scan import descriptors to detect packer-injected imports.
	// Packers often add a small import table (e.g., 4 kernel32 functions)
	// at an unusual RVA far from the real imports.
	ImportScanEntry importScan[32];
	int importScanCount = 0;
	int packerSuspectCount = 0;
	int totalFuncsAll = 0;

	{
		PIMAGE_IMPORT_DESCRIPTOR scanDesc = importDesc;
		while((BYTE*)scanDesc < (BYTE*)pfile + hFileSize - sizeof(IMAGE_IMPORT_DESCRIPTOR) && importScanCount < 32){
			if(scanDesc->TimeDateStamp == 0 && scanDesc->Name == 0)
				break;
			if(scanDesc->Name == 0 || scanDesc->Name >= (DWORD)hFileSize)
				break;

			DWORD sFtRVA = scanDesc->FirstThunk;
			if(sFtRVA == 0 || sFtRVA >= (DWORD)hFileSize){
				scanDesc++;
				continue;
			}

			importScan[importScanCount].ftRVA = sFtRVA;
			importScan[importScanCount].isPackerSuspect = false;
			importScan[importScanCount].packerScore = 0;

			char *dName = pfile + scanDesc->Name;
			if((BYTE*)dName >= (BYTE*)pfile && (BYTE*)dName < (BYTE*)pfile + hFileSize){
				int di = 0;
				for(; dName[di] != 0 && di < MAX_PATH - 1; di++)
					importScan[importScanCount].dllName[di] = (char)tolower((unsigned char)dName[di]);
				importScan[importScanCount].dllName[di] = 0;
			} else {
				importScan[importScanCount].dllName[0] = 0;
			}

			int funcCount = 0;
			if(LoadedPe==TRUE){
				PIMAGE_THUNK_DATA32 cntThk = (PIMAGE_THUNK_DATA32)(sFtRVA + (DWORD)pfile);
				for(int t=0; t<2000; t++){
					if((BYTE*)cntThk >= (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)) break;
					if(cntThk->u1.AddressOfData == 0) break;
					funcCount++;
					cntThk++;
				}
			}
			else if(LoadedPe64==TRUE){
				PIMAGE_THUNK_DATA64 cntThk64 = (PIMAGE_THUNK_DATA64)(sFtRVA + (DWORD)pfile);
				for(int t=0; t<2000; t++){
					if((BYTE*)cntThk64 >= (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)) break;
					if(cntThk64->u1.AddressOfData == 0) break;
					funcCount++;
					cntThk64++;
				}
			}
			importScan[importScanCount].functionCount = funcCount;
			totalFuncsAll += funcCount;
			importScanCount++;
			scanDesc++;
		}
	}

	// Score each descriptor with heuristics
	if(importScanCount > 1){
		// Compute median ftRVA
		DWORD ftRVAs[32];
		for(int i=0; i<importScanCount; i++) ftRVAs[i] = importScan[i].ftRVA;
		for(int i=0; i<importScanCount-1; i++){
			for(int j=i+1; j<importScanCount; j++){
				if(ftRVAs[j] < ftRVAs[i]){ DWORD tmp = ftRVAs[i]; ftRVAs[i] = ftRVAs[j]; ftRVAs[j] = tmp; }
			}
		}
		DWORD medianRVA = ftRVAs[importScanCount / 2];

		// Determine last section range
		IMAGE_SECTION_HEADER *pSec;
		if(LoadedPe==TRUE)
			pSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
		else
			pSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
		DWORD lastSecVA = 0, lastSecEnd = 0;
		if(NumberOfSections > 0){
			IMAGE_SECTION_HEADER *lastSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(pSec) + (NumberOfSections - 1) * sizeof(IMAGE_SECTION_HEADER));
			lastSecVA = lastSec->VirtualAddress;
			lastSecEnd = lastSecVA + lastSec->Misc.VirtualSize;
		}

		// Check if ALL descriptors have high absolute RVA (packer relocated entire IAT)
		// Normal PEs have import tables at low RVAs (< 0x100000)
		bool allHighRVA = true;
		for(int i=0; i<importScanCount; i++){
			if(importScan[i].ftRVA <= 0x100000){ allHighRVA = false; break; }
		}

		for(int i=0; i<importScanCount; i++){
			int score = 0;
			DWORD dist = (importScan[i].ftRVA > medianRVA) ?
				importScan[i].ftRVA - medianRVA : medianRVA - importScan[i].ftRVA;

			if(dist > 0x100000) score += 3;
			else if(dist > 0x50000) score += 2;
			else if(dist > 0x10000) score += 1;

			if(importScan[i].functionCount <= 2) score += 2;
			else if(importScan[i].functionCount <= 5) score += 1;

			if(NumberOfSections > 2 && importScan[i].ftRVA >= lastSecVA && importScan[i].ftRVA < lastSecEnd)
				score += 2;

			if(totalFuncsAll > 0 && importScan[i].functionCount * 100 / totalFuncsAll < 10)
				score += 1;

			// All imports at high RVAs suggests packer relocated entire import table
			if(allHighRVA) score += 1;

			importScan[i].packerScore = score;
			if(score >= 4){
				importScan[i].isPackerSuspect = true;
				packerSuspectCount++;
				wsprintf(InfoText,"IAT Rebuild: PACKER SUSPECT: %s (ftRVA=%08X, %d funcs, score=%d)",
					importScan[i].dllName, importScan[i].ftRVA, importScan[i].functionCount, score);
				OutDebug(hWnd,InfoText);
			}
		}
	}

	// =====================================================
	// Phase 2: Find free space in the file for new hint/name entries
	// =====================================================
	DWORD freeSpaceOffset = 0;
	DWORD freeSpaceSize = 0;

	// Reset section_header pointer
	if(LoadedPe==TRUE){
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
	}
	else if(LoadedPe64==TRUE){
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
	}

	// Strategy 1: Use PE header slack space
	// After rebuild, SizeOfHeaders = 0x1000 and first section starts at VA 0x1000.
	// The section table ends much earlier, leaving a large gap of unused space.
	// This is the classic approach used by import reconstruction tools (ImpREC, Scylla).
	{
		// Calculate end of section table
		IMAGE_SECTION_HEADER *lastSecEntry = (IMAGE_SECTION_HEADER *)((UCHAR *)(sec) + NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
		DWORD headerSlackStart = (DWORD)((BYTE*)lastSecEntry - (BYTE*)pfile);
		// Align to DWORD boundary
		headerSlackStart = (headerSlackStart + 3) & ~3;

		// First section starts at VirtualAddress (after rebuild, PointerToRawData == VirtualAddress)
		DWORD firstSectionStart = 0x1000; // SizeOfHeaders after rebuild
		if(sec->VirtualAddress > 0){
			firstSectionStart = sec->VirtualAddress;
		}

		if(firstSectionStart > headerSlackStart + 64 && firstSectionStart <= (DWORD)hFileSize){
			freeSpaceOffset = headerSlackStart;
			freeSpaceSize = firstSectionStart - headerSlackStart;
			wsprintf(InfoText,"IAT Rebuild: Found %d bytes in PE header slack at offset %08X", freeSpaceSize, freeSpaceOffset);
			OutDebug(hWnd,InfoText);
		}
	}

	// Strategy 2: If header slack is not enough, scan section tails for zero padding
	if(freeSpaceSize < 64){
		IMAGE_SECTION_HEADER *scanSec = sec;
		for(DWORD s=0; s<NumberOfSections; s++){
			// After rebuild, PointerToRawData == VirtualAddress
			DWORD secFileStart = scanSec->VirtualAddress;
			DWORD secFileEnd = secFileStart + scanSec->Misc.VirtualSize;
			if(secFileEnd > (DWORD)hFileSize)
				secFileEnd = (DWORD)hFileSize;

			// Scan backwards from end of section to find where non-zero data ends
			DWORD lastNonZero = secFileStart;
			if(secFileEnd > secFileStart){
				for(DWORD off = secFileEnd - 1; off > secFileStart; off--){
					if((BYTE)pfile[off] != 0){
						lastNonZero = off + 1;
						break;
					}
				}
			}
			// Align to DWORD boundary
			lastNonZero = (lastNonZero + 3) & ~3;

			DWORD thisSpace = 0;
			if(secFileEnd > lastNonZero + 64)
				thisSpace = secFileEnd - lastNonZero;

			if(thisSpace > freeSpaceSize){
				freeSpaceOffset = lastNonZero;
				freeSpaceSize = thisSpace;
			}
			scanSec = (IMAGE_SECTION_HEADER *)((UCHAR *)(scanSec)+sizeof(IMAGE_SECTION_HEADER));
		}
		if(freeSpaceSize >= 64){
			wsprintf(InfoText,"IAT Rebuild: Found %d bytes in section tail at offset %08X", freeSpaceSize, freeSpaceOffset);
			OutDebug(hWnd,InfoText);
		}
	}

	if(freeSpaceSize < 64){
		OutDebug(hWnd,"IAT Rebuild: Not enough free space found for hint/name entries. Skipping IAT rebuild.");
		return;
	}

	// =====================================================
	// Phase 3 & 4: For each DLL, resolve functions and write new entries
	// =====================================================
	DWORD writeOffset = freeSpaceOffset; // Current write position for new hint/name entries
	DWORD spaceEnd = freeSpaceOffset + freeSpaceSize;
	int totalResolved = 0;
	int totalUnresolved = 0;
	int dllCount = 0;

	// Walk import descriptors - two-pass processing:
	// Pass 0: Process non-suspect imports (real IAT gets priority for free space)
	// Pass 1: Process packer-suspect imports (best-effort)
	for(int pass=0; pass<2; pass++){
	if(pass==1 && packerSuspectCount==0) break;

	PIMAGE_IMPORT_DESCRIPTOR curDesc = importDesc;

	while((BYTE*)curDesc < (BYTE*)pfile + hFileSize - sizeof(IMAGE_IMPORT_DESCRIPTOR)){
		if(curDesc->TimeDateStamp == 0 && curDesc->Name == 0)
			break;
		if(curDesc->Name == 0 || curDesc->Name >= (DWORD)hFileSize)
			break;

		DWORD ftRVA = curDesc->FirstThunk;
		if(ftRVA == 0 || ftRVA >= (DWORD)hFileSize){
			curDesc++;
			continue;
		}

		char *dllName = pfile + curDesc->Name;
		// Validate DLL name pointer
		if((BYTE*)dllName < (BYTE*)pfile || (BYTE*)dllName >= (BYTE*)pfile + hFileSize){
			curDesc++;
			continue;
		}

		// Look up packer suspect status from Phase 1b scan
		bool isPacker = false;
		for(int si=0; si<importScanCount; si++){
			if(importScan[si].ftRVA == ftRVA){
				isPacker = importScan[si].isPackerSuspect;
				break;
			}
		}

		// Pass 0: skip packer suspects; Pass 1: skip non-suspects
		if(pass==0 && isPacker){ curDesc++; continue; }
		if(pass==1 && !isPacker){ curDesc++; continue; }

		dllCount++;

		// Normalize DLL name to lowercase (packers often use mixed case like KeRnEl32.dLL)
		char normalizedDll[MAX_PATH];
		{
			int di = 0;
			for(; dllName[di] != 0 && di < MAX_PATH - 1; di++){
				normalizedDll[di] = (char)tolower((unsigned char)dllName[di]);
			}
			normalizedDll[di] = 0;
		}

		wsprintf(InfoText,"IAT Rebuild: Processing %s", normalizedDll);
		OutDebug(hWnd,InfoText);

		// =====================================================
		// Phase 3a: Try OriginalFirstThunk (ILT) first
		// =====================================================
		// The Import Lookup Table often survives packing/dumping intact.
		// If OFT is valid, we can read original hint/name RVAs directly.
		DWORD oftRVA = curDesc->OriginalFirstThunk;
		if(oftRVA != 0 && oftRVA < (DWORD)hFileSize && oftRVA != ftRVA){
			bool oftValid = true;
			int oftCount = 0;

			// Validate OFT entries - they should point to valid IMAGE_IMPORT_BY_NAME structures
			if(LoadedPe==TRUE){
				PIMAGE_THUNK_DATA32 oftThk = (PIMAGE_THUNK_DATA32)(oftRVA + (DWORD)pfile);
				PIMAGE_THUNK_DATA32 iatThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
				while((BYTE*)oftThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
					if(oftThk->u1.AddressOfData == 0) break;
					DWORD hintRVA = oftThk->u1.AddressOfData;
					// Check for ordinal import (high bit set)
					if(hintRVA & 0x80000000){ oftThk++; iatThk++; oftCount++; continue; }
					// Validate: hintRVA should point within the file to a hint + name string
					if(hintRVA >= (DWORD)hFileSize){ oftValid = false; break; }
					// Check that the hint/name area has printable ASCII
					char *namePtr = pfile + hintRVA + sizeof(WORD);
					if((BYTE*)namePtr >= (BYTE*)pfile + hFileSize - 4){ oftValid = false; break; }
					if(namePtr[0] < 0x20 || namePtr[0] > 0x7E){ oftValid = false; break; }
					oftThk++;
					iatThk++;
					oftCount++;
				}
			}
			else if(LoadedPe64==TRUE){
				PIMAGE_THUNK_DATA64 oftThk64 = (PIMAGE_THUNK_DATA64)(oftRVA + (DWORD)pfile);
				while((BYTE*)oftThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
					if(oftThk64->u1.AddressOfData == 0) break;
					ULONGLONG hintRVA = oftThk64->u1.AddressOfData;
					if(hintRVA & 0x8000000000000000ULL){ oftThk64++; oftCount++; continue; }
					if(hintRVA >= (DWORD)hFileSize){ oftValid = false; break; }
					char *namePtr = pfile + (DWORD)hintRVA + sizeof(WORD);
					if((BYTE*)namePtr >= (BYTE*)pfile + hFileSize - 4){ oftValid = false; break; }
					if(namePtr[0] < 0x20 || namePtr[0] > 0x7E){ oftValid = false; break; }
					oftThk64++;
					oftCount++;
				}
			}

			if(oftValid && oftCount > 0){
				wsprintf(InfoText,"IAT Rebuild: OFT intact for %s (%d entries), using ILT to restore IAT", normalizedDll, oftCount);
				OutDebug(hWnd,InfoText);

				// Copy hint/name RVAs from OFT to IAT (FirstThunk)
				if(LoadedPe==TRUE){
					PIMAGE_THUNK_DATA32 oftThk = (PIMAGE_THUNK_DATA32)(oftRVA + (DWORD)pfile);
					PIMAGE_THUNK_DATA32 iatThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
					while((BYTE*)oftThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
						if(oftThk->u1.AddressOfData == 0) break;
						DWORD hintRVA = oftThk->u1.AddressOfData;
						iatThk->u1.AddressOfData = hintRVA;
						// Log the function name
						if(!(hintRVA & 0x80000000) && hintRVA < (DWORD)hFileSize){
							char *namePtr = pfile + hintRVA + sizeof(WORD);
							wsprintf(InfoText,"  %s!%s (from OFT)", normalizedDll, namePtr);
							OutDebug(hWnd,InfoText);
						}
						totalResolved++;
						oftThk++;
						iatThk++;
					}
				}
				else if(LoadedPe64==TRUE){
					PIMAGE_THUNK_DATA64 oftThk64 = (PIMAGE_THUNK_DATA64)(oftRVA + (DWORD)pfile);
					PIMAGE_THUNK_DATA64 iatThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
					while((BYTE*)oftThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
						if(oftThk64->u1.AddressOfData == 0) break;
						ULONGLONG hintRVA = oftThk64->u1.AddressOfData;
						iatThk64->u1.AddressOfData = hintRVA;
						if(!(hintRVA & 0x8000000000000000ULL) && hintRVA < (DWORD)hFileSize){
							char *namePtr = pfile + (DWORD)hintRVA + sizeof(WORD);
							wsprintf(InfoText,"  %s!%s (from OFT)", normalizedDll, namePtr);
							OutDebug(hWnd,InfoText);
						}
						totalResolved++;
						oftThk64++;
						iatThk64++;
					}
				}
				curDesc++;
				continue; // Skip to next DLL, this one is fully resolved via OFT
			}
		}

		// Open the DLL as a flat file from disk and map it for reading.
		// We cannot use LoadLibrary/GetModuleHandle because when analyzing
		// a 32-bit dump from a 64-bit process, those return the 64-bit DLL
		// which has completely different export RVAs.
		HANDLE hDllFile = INVALID_HANDLE_VALUE;
		HANDLE hDllMapping = NULL;
		BYTE *dllFileBase = NULL;
		char dllFilePath[MAX_PATH];

		// Search order: SysWOW64 (for 32-bit dumps), System32, SearchPath
		if(LoadedPe==TRUE){
			// 32-bit dump: try SysWOW64 first for the 32-bit DLL
			char winDir[MAX_PATH];
			GetWindowsDirectoryA(winDir, MAX_PATH);
			wsprintf(dllFilePath, "%s\\SysWOW64\\%s", winDir, normalizedDll);
			hDllFile = CreateFileA(dllFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		}
		if(hDllFile == INVALID_HANDLE_VALUE){
			// Try System32
			char sysDir[MAX_PATH];
			GetSystemDirectoryA(sysDir, MAX_PATH);
			wsprintf(dllFilePath, "%s\\%s", sysDir, normalizedDll);
			hDllFile = CreateFileA(dllFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		}
		if(hDllFile == INVALID_HANDLE_VALUE){
			// Try SearchPath (standard DLL search order)
			if(SearchPathA(NULL, normalizedDll, ".dll", MAX_PATH, dllFilePath, NULL)){
				hDllFile = CreateFileA(dllFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			}
		}
		if(hDllFile == INVALID_HANDLE_VALUE){
			wsprintf(InfoText,"IAT Rebuild: WARNING - Could not find %s on disk", normalizedDll);
			OutDebug(hWnd,InfoText);
			curDesc++;
			continue;
		}

		// Map the DLL file into memory as a flat file (read-only)
		hDllMapping = CreateFileMappingA(hDllFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if(!hDllMapping){
			CloseHandle(hDllFile);
			wsprintf(InfoText,"IAT Rebuild: WARNING - Could not map %s", normalizedDll);
			OutDebug(hWnd,InfoText);
			curDesc++;
			continue;
		}
		dllFileBase = (BYTE*)MapViewOfFile(hDllMapping, FILE_MAP_READ, 0, 0, 0);
		if(!dllFileBase){
			CloseHandle(hDllMapping);
			CloseHandle(hDllFile);
			wsprintf(InfoText,"IAT Rebuild: WARNING - Could not map view of %s", normalizedDll);
			OutDebug(hWnd,InfoText);
			curDesc++;
			continue;
		}

		// Parse the DLL's PE headers from the flat file.
		// Use raw byte offsets from the PE spec for architecture independence.

		// Validate DOS signature
		if(*(WORD*)dllFileBase != IMAGE_DOS_SIGNATURE){
			wsprintf(InfoText,"IAT Rebuild: %s has invalid DOS signature", normalizedDll);
			OutDebug(hWnd,InfoText);
			UnmapViewOfFile(dllFileBase);
			CloseHandle(hDllMapping);
			CloseHandle(hDllFile);
			curDesc++;
			continue;
		}

		LONG dllLfanew = *(LONG*)(dllFileBase + 0x3C);
		BYTE *dllNtBase = dllFileBase + dllLfanew;

		// Validate PE signature
		if(*(DWORD*)dllNtBase != IMAGE_NT_SIGNATURE){
			wsprintf(InfoText,"IAT Rebuild: %s has invalid PE signature", normalizedDll);
			OutDebug(hWnd,InfoText);
			UnmapViewOfFile(dllFileBase);
			CloseHandle(hDllMapping);
			CloseHandle(hDllFile);
			curDesc++;
			continue;
		}

		// OptionalHeader starts at NT + 4 (signature) + 20 (FileHeader) = NT + 24
		BYTE *dllOptHeader = dllNtBase + 24;
		WORD dllMagic = *(WORD*)dllOptHeader;

		// Read DataDirectory[0] (Export Directory) using PE-spec offsets:
		// PE32 (0x10B): DataDirectory starts at OptionalHeader + 96
		// PE64 (0x20B): DataDirectory starts at OptionalHeader + 112
		DWORD exportDirRVA, exportDirSize;
		if(dllMagic == 0x20B){
			exportDirRVA = *(DWORD*)(dllOptHeader + 112);
			exportDirSize = *(DWORD*)(dllOptHeader + 116);
		}
		else{
			exportDirRVA = *(DWORD*)(dllOptHeader + 96);
			exportDirSize = *(DWORD*)(dllOptHeader + 100);
		}

		if(exportDirRVA == 0 || exportDirSize == 0){
			wsprintf(InfoText,"IAT Rebuild: %s has no export table", normalizedDll);
			OutDebug(hWnd,InfoText);
			UnmapViewOfFile(dllFileBase);
			CloseHandle(hDllMapping);
			CloseHandle(hDllFile);
			curDesc++;
			continue;
		}

		// For a flat file on disk, we must convert RVAs to file offsets
		// using the section table (sections aren't mapped at virtual addresses)
		IMAGE_EXPORT_DIRECTORY *dllExportDir = (IMAGE_EXPORT_DIRECTORY*)(dllFileBase + DllRvaToFileOffset(dllFileBase, exportDirRVA));
		DWORD *dllFunctions = (DWORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllExportDir->AddressOfFunctions));
		DWORD *dllNames = (DWORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllExportDir->AddressOfNames));
		WORD *dllOrdinals = (WORD*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllExportDir->AddressOfNameOrdinals));

		// Phase 3: Determine the DLL base address using the first thunk
		// Try: candidate_base = first_thunk_value - export_rva
		// Verify against all other thunks
		DWORD_PTR candidateBase = 0;
		bool baseFound = false;

		if(LoadedPe==TRUE){
			PIMAGE_THUNK_DATA32 firstThunk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
			if((BYTE*)firstThunk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
				DWORD firstVal = firstThunk->u1.Function;

				// Try each export RVA to find the base
				for(DWORD e=0; e<dllExportDir->NumberOfFunctions && !baseFound; e++){
					if(dllFunctions[e] == 0) continue;
					DWORD_PTR testBase = firstVal - dllFunctions[e];

					// Verify: check that multiple other thunks resolve with this base
					int verified = 0;
					int checked = 0;
					PIMAGE_THUNK_DATA32 verifyThunk = firstThunk;
					for(int t=0; t<500; t++){
						if((BYTE*)verifyThunk >= (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32))
							break;
						if(verifyThunk->u1.AddressOfData == 0)
							break;
						checked++;
						DWORD verifyVal = (DWORD)verifyThunk->u1.Function;
						DWORD verifyRVA = (DWORD)(verifyVal - testBase);
						// Check if this RVA exists in the export table
						for(DWORD v=0; v<dllExportDir->NumberOfFunctions; v++){
							if(dllFunctions[v] == verifyRVA){
								verified++;
								break;
							}
						}
						verifyThunk++;
					}
					// If most thunks resolve, we found the base
					if(checked > 0 && verified >= (checked + 1) / 2){
						candidateBase = testBase;
						baseFound = true;
					}
				}
			}
		}
		else if(LoadedPe64==TRUE){
			PIMAGE_THUNK_DATA64 firstThunk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
			if((BYTE*)firstThunk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
				ULONGLONG firstVal = firstThunk64->u1.Function;

				for(DWORD e=0; e<dllExportDir->NumberOfFunctions && !baseFound; e++){
					if(dllFunctions[e] == 0) continue;
					ULONGLONG testBase = firstVal - dllFunctions[e];

					int verified = 0;
					int checked = 0;
					PIMAGE_THUNK_DATA64 verifyThunk64 = firstThunk64;
					for(int t=0; t<500; t++){
						if((BYTE*)verifyThunk64 >= (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64))
							break;
						if(verifyThunk64->u1.AddressOfData == 0)
							break;
						checked++;
						ULONGLONG verifyVal = verifyThunk64->u1.Function;
						DWORD verifyRVA = (DWORD)(verifyVal - testBase);
						for(DWORD v=0; v<dllExportDir->NumberOfFunctions; v++){
							if(dllFunctions[v] == verifyRVA){
								verified++;
								break;
							}
						}
						verifyThunk64++;
					}
					if(checked > 0 && verified >= (checked + 1) / 2){
						candidateBase = (DWORD_PTR)testBase;
						baseFound = true;
					}
				}
			}
		}

		if(!baseFound){
			wsprintf(InfoText,"IAT Rebuild: Could not determine base for %s, skipping", normalizedDll);
			OutDebug(hWnd,InfoText);
			UnmapViewOfFile(dllFileBase);
			CloseHandle(hDllMapping);
			CloseHandle(hDllFile);
			curDesc++;
			continue;
		}

		// Phase 4: Resolve all functions and write new hint/name entries
		// Clear OriginalFirstThunk (loader uses FirstThunk when OFT is 0)
		curDesc->OriginalFirstThunk = 0;

		if(LoadedPe==TRUE){
			PIMAGE_THUNK_DATA32 thk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
			while((BYTE*)thk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
				if(thk->u1.AddressOfData == 0)
					break;

				DWORD thunkVal = (DWORD)thk->u1.Function;
				DWORD expRVA = (DWORD)(thunkVal - candidateBase);
				char *funcName = NULL;

				// Find the function name by matching the export RVA
				for(DWORD e=0; e<dllExportDir->NumberOfFunctions; e++){
					if(dllFunctions[e] == expRVA){
						for(DWORD n=0; n<dllExportDir->NumberOfNames; n++){
							if(dllOrdinals[n] == e){
								funcName = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllNames[n]));
								break;
							}
						}
						break;
					}
				}

				if(funcName != NULL){
					DWORD nameLen = 0;
					while(funcName[nameLen] != 0) nameLen++;
					DWORD entrySize = sizeof(WORD) + nameLen + 1;
					entrySize = (entrySize + 3) & ~3;

					if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
						*(WORD*)(pfile + writeOffset) = 0;
						for(DWORD c=0; c<=nameLen; c++){
							pfile[writeOffset + sizeof(WORD) + c] = funcName[c];
						}
						thk->u1.AddressOfData = writeOffset;
						wsprintf(InfoText,"  %s!%s", normalizedDll, funcName);
						OutDebug(hWnd,InfoText);
						totalResolved++;
						writeOffset += entrySize;
					}
				}
				// else: leave thunk unchanged for now, Phase 4b will try forwarded exports
				thk++;
			}
		}
		else if(LoadedPe64==TRUE){
			PIMAGE_THUNK_DATA64 thk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
			while((BYTE*)thk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
				if(thk64->u1.AddressOfData == 0)
					break;

				ULONGLONG thunkVal = thk64->u1.Function;
				DWORD expRVA = (DWORD)(thunkVal - candidateBase);
				char *funcName = NULL;

				for(DWORD e=0; e<dllExportDir->NumberOfFunctions; e++){
					if(dllFunctions[e] == expRVA){
						for(DWORD n=0; n<dllExportDir->NumberOfNames; n++){
							if(dllOrdinals[n] == e){
								funcName = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllNames[n]));
								break;
							}
						}
						break;
					}
				}

				if(funcName != NULL){
					DWORD nameLen = 0;
					while(funcName[nameLen] != 0) nameLen++;
					DWORD entrySize = sizeof(WORD) + nameLen + 1;
					entrySize = (entrySize + 3) & ~3;

					if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
						*(WORD*)(pfile + writeOffset) = 0;
						for(DWORD c=0; c<=nameLen; c++){
							pfile[writeOffset + sizeof(WORD) + c] = funcName[c];
						}
						thk64->u1.AddressOfData = (ULONGLONG)writeOffset;
						wsprintf(InfoText,"  %s!%s", normalizedDll, funcName);
						OutDebug(hWnd,InfoText);
						totalResolved++;
						writeOffset += entrySize;
					}
				}
				thk64++;
			}
		}

		// =====================================================
		// Phase 4b: Resolve forwarded exports
		// =====================================================
		// On modern Windows, kernel32 forwards many functions to kernelbase.dll.
		// The IAT thunk values point into the target DLL, not the importing DLL.
		// We detect forwarded exports, map the target DLL, do base detection,
		// and resolve the original function names.
		{
			// Count remaining unresolved thunks (still containing resolved addresses)
			int unresolvedLeft = 0;
			if(LoadedPe==TRUE){
				PIMAGE_THUNK_DATA32 scanThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
				while((BYTE*)scanThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
					if(scanThk->u1.AddressOfData == 0) break;
					if(scanThk->u1.AddressOfData > SizeOfImage) unresolvedLeft++;
					scanThk++;
				}
			}
			else if(LoadedPe64==TRUE){
				PIMAGE_THUNK_DATA64 scanThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
				while((BYTE*)scanThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
					if(scanThk64->u1.AddressOfData == 0) break;
					if(scanThk64->u1.AddressOfData > SizeOfImage) unresolvedLeft++;
					scanThk64++;
				}
			}

			if(unresolvedLeft > 0){
				wsprintf(InfoText,"IAT Rebuild: %d thunks unresolved, checking forwarded exports...", unresolvedLeft);
				OutDebug(hWnd,InfoText);

				// Debug: print the actual unresolved thunk values
				if(LoadedPe==TRUE){
					PIMAGE_THUNK_DATA32 dbgThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
					while((BYTE*)dbgThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
						if(dbgThk->u1.AddressOfData == 0) break;
						if(dbgThk->u1.AddressOfData > SizeOfImage){
							wsprintf(InfoText,"IAT Rebuild:   Unresolved thunk value: %08X", dbgThk->u1.AddressOfData);
							OutDebug(hWnd,InfoText);
						}
						dbgThk++;
					}
				}

				// Collect forwarded exports grouped by target DLL.
				// Process one target DLL at a time.
				char processedTargets[16][MAX_PATH];
				int numProcessedTargets = 0;

				for(DWORD e=0; e<dllExportDir->NumberOfFunctions && unresolvedLeft > 0; e++){
					DWORD fwdRVA = dllFunctions[e];
					if(fwdRVA == 0) continue;
					// A forwarded export has its RVA within the export directory range
					if(fwdRVA < exportDirRVA || fwdRVA >= exportDirRVA + exportDirSize) continue;

					// Parse the forwarder string: "TARGETDLL.FuncName"
					char *forwarder = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, fwdRVA));
					char targetDllName[MAX_PATH];
					int dotPos = -1;
					for(int f=0; forwarder[f] != 0 && f < MAX_PATH-5; f++){
						if(forwarder[f] == '.') { dotPos = f; break; }
						targetDllName[f] = (char)tolower((unsigned char)forwarder[f]);
					}
					if(dotPos <= 0) continue;
					targetDllName[dotPos] = 0;
					// Append .dll
					wsprintf(targetDllName + dotPos, ".dll");

					// Skip if we already processed this target DLL
					bool alreadyProcessed = false;
					for(int t=0; t<numProcessedTargets; t++){
						if(strcmp(processedTargets[t], targetDllName) == 0){
							alreadyProcessed = true;
							break;
						}
					}
					if(alreadyProcessed) continue;
					if(numProcessedTargets < 16){
						strcpy(processedTargets[numProcessedTargets], targetDllName);
						numProcessedTargets++;
					}

					// Map the target DLL from disk
					HANDLE hTgtFile = INVALID_HANDLE_VALUE;
					char tgtPath[MAX_PATH];
					if(LoadedPe==TRUE){
						char winDir[MAX_PATH];
						GetWindowsDirectoryA(winDir, MAX_PATH);
						wsprintf(tgtPath, "%s\\SysWOW64\\%s", winDir, targetDllName);
						hTgtFile = CreateFileA(tgtPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
					}
					if(hTgtFile == INVALID_HANDLE_VALUE){
						char sysDir[MAX_PATH];
						GetSystemDirectoryA(sysDir, MAX_PATH);
						wsprintf(tgtPath, "%s\\%s", sysDir, targetDllName);
						hTgtFile = CreateFileA(tgtPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
					}
					if(hTgtFile == INVALID_HANDLE_VALUE) continue;

					HANDLE hTgtMapping = CreateFileMappingA(hTgtFile, NULL, PAGE_READONLY, 0, 0, NULL);
					if(!hTgtMapping){ CloseHandle(hTgtFile); continue; }
					BYTE *tgtBase = (BYTE*)MapViewOfFile(hTgtMapping, FILE_MAP_READ, 0, 0, 0);
					if(!tgtBase){ CloseHandle(hTgtMapping); CloseHandle(hTgtFile); continue; }

					// Parse target DLL's export table
					if(*(WORD*)tgtBase != IMAGE_DOS_SIGNATURE){ UnmapViewOfFile(tgtBase); CloseHandle(hTgtMapping); CloseHandle(hTgtFile); continue; }
					LONG tgtLfanew = *(LONG*)(tgtBase + 0x3C);
					if(*(DWORD*)(tgtBase + tgtLfanew) != IMAGE_NT_SIGNATURE){ UnmapViewOfFile(tgtBase); CloseHandle(hTgtMapping); CloseHandle(hTgtFile); continue; }
					BYTE *tgtOptHdr = tgtBase + tgtLfanew + 24;
					WORD tgtMagic = *(WORD*)tgtOptHdr;
					DWORD tgtExpRVA, tgtExpSize;
					if(tgtMagic == 0x20B){ tgtExpRVA = *(DWORD*)(tgtOptHdr+112); tgtExpSize = *(DWORD*)(tgtOptHdr+116); }
					else{ tgtExpRVA = *(DWORD*)(tgtOptHdr+96); tgtExpSize = *(DWORD*)(tgtOptHdr+100); }

					if(tgtExpRVA == 0 || tgtExpSize == 0){ UnmapViewOfFile(tgtBase); CloseHandle(hTgtMapping); CloseHandle(hTgtFile); continue; }

					IMAGE_EXPORT_DIRECTORY *tgtExpDir = (IMAGE_EXPORT_DIRECTORY*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtExpRVA));
					DWORD *tgtFuncs = (DWORD*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtExpDir->AddressOfFunctions));
					DWORD *tgtNames = (DWORD*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtExpDir->AddressOfNames));
					WORD *tgtOrds = (WORD*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtExpDir->AddressOfNameOrdinals));

					wsprintf(InfoText,"IAT Rebuild: Checking forwarded target %s (%d exports)", targetDllName, tgtExpDir->NumberOfFunctions);
					OutDebug(hWnd,InfoText);

					// Base detection for the target DLL using unresolved thunks
					DWORD_PTR tgtCandidateBase = 0;
					bool tgtBaseFound = false;

					if(LoadedPe==TRUE){
						// Try each unresolved thunk as a seed for base detection
						// Only accept 64KB-aligned bases (DLL base addresses are always 64KB-aligned)
						int bestVerified = 0;
						PIMAGE_THUNK_DATA32 fwdThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
						while((BYTE*)fwdThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
							if(fwdThk->u1.AddressOfData == 0) break;
							DWORD fwdVal = fwdThk->u1.AddressOfData;
							if(fwdVal <= SizeOfImage){ fwdThk++; continue; }

							for(DWORD te=0; te<tgtExpDir->NumberOfFunctions; te++){
								if(tgtFuncs[te] == 0) continue;
								if(tgtFuncs[te] >= tgtExpRVA && tgtFuncs[te] < tgtExpRVA + tgtExpSize) continue;
								DWORD_PTR testBase = fwdVal - tgtFuncs[te];
								if((testBase & 0xFFFF) != 0) continue; // DLL bases are 64KB-aligned
								// Verify against other unresolved thunks
								int verified = 0, checked = 0;
								PIMAGE_THUNK_DATA32 vThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
								while((BYTE*)vThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
									if(vThk->u1.AddressOfData == 0) break;
									DWORD vVal = vThk->u1.AddressOfData;
									if(vVal > SizeOfImage){
										checked++;
										DWORD vExpRVA2 = (DWORD)(vVal - testBase);
										for(DWORD tv=0; tv<tgtExpDir->NumberOfFunctions; tv++){
											if(tgtFuncs[tv] == vExpRVA2){ verified++; break; }
										}
									}
									vThk++;
								}
								if(checked > 0 && verified > bestVerified){
									bestVerified = verified;
									tgtCandidateBase = testBase;
									if(verified == checked) break;
								}
							}
							if(bestVerified == unresolvedLeft) break;
							fwdThk++;
						}
						if(bestVerified > 0){
							wsprintf(InfoText,"IAT Rebuild:   Base candidate %08X (verified %d/%d)", (DWORD)tgtCandidateBase, bestVerified, unresolvedLeft);
							OutDebug(hWnd,InfoText);
						}
						tgtBaseFound = (bestVerified > 0);
					}
					else if(LoadedPe64==TRUE){
						int bestVerified64 = 0;
						PIMAGE_THUNK_DATA64 fwdThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
						while((BYTE*)fwdThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
							if(fwdThk64->u1.AddressOfData == 0) break;
							ULONGLONG fwdVal64 = fwdThk64->u1.AddressOfData;
							if(fwdVal64 <= SizeOfImage){ fwdThk64++; continue; }
							for(DWORD te=0; te<tgtExpDir->NumberOfFunctions; te++){
								if(tgtFuncs[te] == 0) continue;
								if(tgtFuncs[te] >= tgtExpRVA && tgtFuncs[te] < tgtExpRVA + tgtExpSize) continue;
								ULONGLONG testBase64 = fwdVal64 - tgtFuncs[te];
								if((testBase64 & 0xFFFF) != 0) continue; // DLL bases are 64KB-aligned
								int verified = 0, checked = 0;
								PIMAGE_THUNK_DATA64 vThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
								while((BYTE*)vThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
									if(vThk64->u1.AddressOfData == 0) break;
									if(vThk64->u1.AddressOfData > SizeOfImage){
										checked++;
										DWORD vExpRVA2 = (DWORD)(vThk64->u1.AddressOfData - testBase64);
										for(DWORD tv=0; tv<tgtExpDir->NumberOfFunctions; tv++){
											if(tgtFuncs[tv] == vExpRVA2){ verified++; break; }
										}
									}
									vThk64++;
								}
								if(checked > 0 && verified > bestVerified64){
									bestVerified64 = verified;
									tgtCandidateBase = (DWORD_PTR)testBase64;
									if(verified == checked) break;
								}
							}
							if(bestVerified64 == unresolvedLeft) break; // found perfect match
							fwdThk64++;
						}
						if(bestVerified64 > 0){
							wsprintf(InfoText,"IAT Rebuild:   Base candidate %I64X (verified %d/%d)", tgtCandidateBase, bestVerified64, unresolvedLeft);
							OutDebug(hWnd,InfoText);
						}
						tgtBaseFound = (bestVerified64 > 0);
					}

					// =====================================================
					// Forward resolution fallbacks (when brute-force fails)
					// =====================================================
					bool fallbackCUsed = false;

					// Fallback A: Try preferred ImageBase from target DLL PE header
					if(!tgtBaseFound){
						ULONGLONG preferredBase = 0;
						if(tgtMagic == 0x20B)
							preferredBase = *(ULONGLONG*)(tgtOptHdr + 24);
						else
							preferredBase = (ULONGLONG)(*(DWORD*)(tgtOptHdr + 28));
						int fbAVerified = 0;
						if(LoadedPe==TRUE){
							PIMAGE_THUNK_DATA32 vThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
							while((BYTE*)vThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
								if(vThk->u1.AddressOfData == 0) break;
								if(vThk->u1.AddressOfData > SizeOfImage){
									DWORD vRVA = (DWORD)((ULONGLONG)vThk->u1.AddressOfData - preferredBase);
									for(DWORD tv=0; tv<tgtExpDir->NumberOfFunctions; tv++){
										if(tgtFuncs[tv] == vRVA){ fbAVerified++; break; }
									}
								}
								vThk++;
							}
						}
						else if(LoadedPe64==TRUE){
							PIMAGE_THUNK_DATA64 vThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
							while((BYTE*)vThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
								if(vThk64->u1.AddressOfData == 0) break;
								if(vThk64->u1.AddressOfData > SizeOfImage){
									DWORD vRVA = (DWORD)(vThk64->u1.AddressOfData - preferredBase);
									for(DWORD tv=0; tv<tgtExpDir->NumberOfFunctions; tv++){
										if(tgtFuncs[tv] == vRVA){ fbAVerified++; break; }
									}
								}
								vThk64++;
							}
						}
						if(fbAVerified > 0){
							tgtCandidateBase = (DWORD_PTR)preferredBase;
							tgtBaseFound = true;
							wsprintf(InfoText,"IAT Rebuild:   Fallback A: preferred base %08X (verified %d)", (DWORD)preferredBase, fbAVerified);
							OutDebug(hWnd,InfoText);
						}
					}

					// Fallback B: GetModuleHandle for loaded module base (PE32 only)
					if(!tgtBaseFound && LoadedPe==TRUE){
						HMODULE hTgtMod = GetModuleHandleA(targetDllName);
						if(hTgtMod){
							DWORD_PTR modBase = (DWORD_PTR)hTgtMod;
							int fbBVerified = 0;
							PIMAGE_THUNK_DATA32 vThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
							while((BYTE*)vThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
								if(vThk->u1.AddressOfData == 0) break;
								if(vThk->u1.AddressOfData > SizeOfImage){
									DWORD vRVA = (DWORD)(vThk->u1.AddressOfData - modBase);
									for(DWORD tv=0; tv<tgtExpDir->NumberOfFunctions; tv++){
										if(tgtFuncs[tv] == vRVA){ fbBVerified++; break; }
									}
								}
								vThk++;
							}
							if(fbBVerified > 0){
								tgtCandidateBase = modBase;
								tgtBaseFound = true;
								wsprintf(InfoText,"IAT Rebuild:   Fallback B: in-process base %08X (verified %d)", (DWORD)modBase, fbBVerified);
								OutDebug(hWnd,InfoText);
							}
						}
					}

					// Fallback C: Name-based resolution without base detection
					// When DLL versions differ, resolve by matching function names
					// through the forwarder map instead of relying on RVA-based base detection
					if(!tgtBaseFound){
						wsprintf(InfoText,"IAT Rebuild:   Fallback C: name-based resolution for %s", targetDllName);
						OutDebug(hWnd,InfoText);

						// Build forwarder map from source DLL: for each forwarded export
						// targeting the current target DLL, store {targetFuncName, originalName}
						int fwdMapMax = 1024;
						char *fwdMapTgt = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fwdMapMax * 64);
						char *fwdMapOrig = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fwdMapMax * 64);
						int fwdMapCount = 0;

						if(fwdMapTgt && fwdMapOrig){
							for(DWORD oe=0; oe<dllExportDir->NumberOfFunctions && fwdMapCount < fwdMapMax; oe++){
								DWORD oeRVA = dllFunctions[oe];
								if(oeRVA == 0) continue;
								if(oeRVA < exportDirRVA || oeRVA >= exportDirRVA + exportDirSize) continue;
								char *fwd = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, oeRVA));
								char tmpDll[MAX_PATH];
								int dp = -1;
								for(int fi=0; fwd[fi]!=0 && fi < MAX_PATH-5; fi++){
									if(fwd[fi] == '.'){ dp = fi; break; }
									tmpDll[fi] = (char)tolower((unsigned char)fwd[fi]);
								}
								if(dp <= 0) continue;
								tmpDll[dp] = 0;
								char tmpFull[MAX_PATH];
								wsprintf(tmpFull, "%s.dll", tmpDll);
								if(_stricmp(tmpFull, targetDllName) != 0) continue;
								char *fwdFunc = &fwd[dp + 1];
								char *origN = NULL;
								for(DWORD on=0; on<dllExportDir->NumberOfNames; on++){
									if(dllOrdinals[on] == oe){
										origN = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllNames[on]));
										break;
									}
								}
								if(!origN) continue;
								strncpy(fwdMapTgt + fwdMapCount*64, fwdFunc, 63);
								strncpy(fwdMapOrig + fwdMapCount*64, origN, 63);
								fwdMapCount++;
							}

							wsprintf(InfoText,"IAT Rebuild:   Forwarder map: %d entries for %s", fwdMapCount, targetDllName);
							OutDebug(hWnd,InfoText);

							if(fwdMapCount > 0){
								// Find best candidate base via cross-verification
								DWORD_PTR bestBase = 0;
								int bestMatches = 0;

								if(LoadedPe==TRUE){
									PIMAGE_THUNK_DATA32 seedThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
									while((BYTE*)seedThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
										if(seedThk->u1.AddressOfData == 0) break;
										DWORD seedVal = seedThk->u1.AddressOfData;
										if(seedVal <= SizeOfImage){ seedThk++; continue; }
										for(DWORD te=0; te<tgtExpDir->NumberOfFunctions; te++){
											if(tgtFuncs[te] == 0) continue;
											if(tgtFuncs[te] >= tgtExpRVA && tgtFuncs[te] < tgtExpRVA + tgtExpSize) continue;
											DWORD_PTR testBase = seedVal - tgtFuncs[te];
											if((testBase & 0xFFFF) != 0) continue;
											if(testBase <= 0x10000 || testBase >= 0x7FFF0000) continue;
											char *teName = NULL;
											for(DWORD tn=0; tn<tgtExpDir->NumberOfNames; tn++){
												if(tgtOrds[tn] == (WORD)te){
													teName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[tn]));
													break;
												}
											}
											if(!teName) continue;
											bool inMap = false;
											for(int fm=0; fm<fwdMapCount; fm++){
												if(strcmp(fwdMapTgt + fm*64, teName) == 0){ inMap = true; break; }
											}
											if(!inMap) continue;
											// Cross-verify against all unresolved thunks
											int matches = 0;
											PIMAGE_THUNK_DATA32 cvThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
											while((BYTE*)cvThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
												if(cvThk->u1.AddressOfData == 0) break;
												if(cvThk->u1.AddressOfData > SizeOfImage){
													DWORD cvRVA = (DWORD)(cvThk->u1.AddressOfData - testBase);
													for(DWORD tv=0; tv<tgtExpDir->NumberOfFunctions; tv++){
														if(tgtFuncs[tv] == cvRVA && !(tgtFuncs[tv] >= tgtExpRVA && tgtFuncs[tv] < tgtExpRVA + tgtExpSize)){
															char *cvName = NULL;
															for(DWORD cn=0; cn<tgtExpDir->NumberOfNames; cn++){
																if(tgtOrds[cn] == (WORD)tv){
																	cvName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[cn]));
																	break;
																}
															}
															if(cvName){
																for(int fm=0; fm<fwdMapCount; fm++){
																	if(strcmp(fwdMapTgt + fm*64, cvName) == 0){ matches++; break; }
																}
															}
															break;
														}
													}
												}
												cvThk++;
											}
											if(matches > bestMatches){
												bestMatches = matches;
												bestBase = testBase;
												if(matches == unresolvedLeft) break;
											}
										}
										if(bestMatches == unresolvedLeft) break;
										seedThk++;
									}
								}
								else if(LoadedPe64==TRUE){
									PIMAGE_THUNK_DATA64 seedThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
									while((BYTE*)seedThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
										if(seedThk64->u1.AddressOfData == 0) break;
										ULONGLONG seedVal64 = seedThk64->u1.AddressOfData;
										if(seedVal64 <= SizeOfImage){ seedThk64++; continue; }
										for(DWORD te=0; te<tgtExpDir->NumberOfFunctions; te++){
											if(tgtFuncs[te] == 0) continue;
											if(tgtFuncs[te] >= tgtExpRVA && tgtFuncs[te] < tgtExpRVA + tgtExpSize) continue;
											ULONGLONG testBase64 = seedVal64 - tgtFuncs[te];
											if((testBase64 & 0xFFFF) != 0) continue;
											if(testBase64 <= 0x10000) continue;
											char *teName = NULL;
											for(DWORD tn=0; tn<tgtExpDir->NumberOfNames; tn++){
												if(tgtOrds[tn] == (WORD)te){
													teName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[tn]));
													break;
												}
											}
											if(!teName) continue;
											bool inMap = false;
											for(int fm=0; fm<fwdMapCount; fm++){
												if(strcmp(fwdMapTgt + fm*64, teName) == 0){ inMap = true; break; }
											}
											if(!inMap) continue;
											int matches = 0;
											PIMAGE_THUNK_DATA64 cvThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
											while((BYTE*)cvThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
												if(cvThk64->u1.AddressOfData == 0) break;
												if(cvThk64->u1.AddressOfData > SizeOfImage){
													DWORD cvRVA = (DWORD)(cvThk64->u1.AddressOfData - testBase64);
													for(DWORD tv=0; tv<tgtExpDir->NumberOfFunctions; tv++){
														if(tgtFuncs[tv] == cvRVA && !(tgtFuncs[tv] >= tgtExpRVA && tgtFuncs[tv] < tgtExpRVA + tgtExpSize)){
															char *cvName = NULL;
															for(DWORD cn=0; cn<tgtExpDir->NumberOfNames; cn++){
																if(tgtOrds[cn] == (WORD)tv){
																	cvName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[cn]));
																	break;
																}
															}
															if(cvName){
																for(int fm=0; fm<fwdMapCount; fm++){
																	if(strcmp(fwdMapTgt + fm*64, cvName) == 0){ matches++; break; }
																}
															}
															break;
														}
													}
												}
												cvThk64++;
											}
											if(matches > bestMatches){
												bestMatches = matches;
												bestBase = (DWORD_PTR)testBase64;
												if(matches == unresolvedLeft) break;
											}
										}
										if(bestMatches == unresolvedLeft) break;
										seedThk64++;
									}
								}

								// Resolve thunks using bestBase + forwarder map
								if(bestMatches > 0){
									wsprintf(InfoText,"IAT Rebuild:   Fallback C: base %08X resolves %d thunks via name matching", (DWORD)bestBase, bestMatches);
									OutDebug(hWnd,InfoText);

									if(LoadedPe==TRUE){
										PIMAGE_THUNK_DATA32 rThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
										while((BYTE*)rThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
											if(rThk->u1.AddressOfData == 0) break;
											DWORD rVal = rThk->u1.AddressOfData;
											if(rVal <= SizeOfImage){ rThk++; continue; }
											DWORD rRVA = (DWORD)(rVal - bestBase);
											char *tgtFuncName = NULL;
											for(DWORD tn=0; tn<tgtExpDir->NumberOfNames; tn++){
												DWORD tIdx = tgtOrds[tn];
												if(tIdx < tgtExpDir->NumberOfFunctions && tgtFuncs[tIdx] == rRVA){
													tgtFuncName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[tn]));
													break;
												}
											}
											char *originalName = NULL;
											if(tgtFuncName){
												for(int fm=0; fm<fwdMapCount; fm++){
													if(strcmp(fwdMapTgt + fm*64, tgtFuncName) == 0){
														originalName = fwdMapOrig + fm*64;
														break;
													}
												}
											}
											if(originalName){
												DWORD nameLen = 0;
												while(originalName[nameLen] != 0) nameLen++;
												DWORD entrySize = (sizeof(WORD) + nameLen + 1 + 3) & ~3;
												if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
													*(WORD*)(pfile + writeOffset) = 0;
													for(DWORD c=0; c<=nameLen; c++)
														pfile[writeOffset + sizeof(WORD) + c] = originalName[c];
													rThk->u1.AddressOfData = writeOffset;
													wsprintf(InfoText,"  %s!%s (via %s, name-matched)", normalizedDll, originalName, targetDllName);
													OutDebug(hWnd,InfoText);
													totalResolved++;
													unresolvedLeft--;
													writeOffset += entrySize;
												}
											}
											rThk++;
										}
									}
									else if(LoadedPe64==TRUE){
										PIMAGE_THUNK_DATA64 rThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
										while((BYTE*)rThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
											if(rThk64->u1.AddressOfData == 0) break;
											if(rThk64->u1.AddressOfData <= SizeOfImage){ rThk64++; continue; }
											DWORD rRVA = (DWORD)(rThk64->u1.AddressOfData - (ULONGLONG)bestBase);
											char *tgtFuncName = NULL;
											for(DWORD tn=0; tn<tgtExpDir->NumberOfNames; tn++){
												DWORD tIdx = tgtOrds[tn];
												if(tIdx < tgtExpDir->NumberOfFunctions && tgtFuncs[tIdx] == rRVA){
													tgtFuncName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[tn]));
													break;
												}
											}
											char *originalName = NULL;
											if(tgtFuncName){
												for(int fm=0; fm<fwdMapCount; fm++){
													if(strcmp(fwdMapTgt + fm*64, tgtFuncName) == 0){
														originalName = fwdMapOrig + fm*64;
														break;
													}
												}
											}
											if(originalName){
												DWORD nameLen = 0;
												while(originalName[nameLen] != 0) nameLen++;
												DWORD entrySize = (sizeof(WORD) + nameLen + 1 + 3) & ~3;
												if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
													*(WORD*)(pfile + writeOffset) = 0;
													for(DWORD c=0; c<=nameLen; c++)
														pfile[writeOffset + sizeof(WORD) + c] = originalName[c];
													rThk64->u1.AddressOfData = (ULONGLONG)writeOffset;
													wsprintf(InfoText,"  %s!%s (via %s, name-matched)", normalizedDll, originalName, targetDllName);
													OutDebug(hWnd,InfoText);
													totalResolved++;
													unresolvedLeft--;
													writeOffset += entrySize;
												}
											}
											rThk64++;
										}
									}
									fallbackCUsed = true;
								}
							}
						}

						if(fwdMapTgt) HeapFree(GetProcessHeap(), 0, fwdMapTgt);
						if(fwdMapOrig) HeapFree(GetProcessHeap(), 0, fwdMapOrig);
					}

					if(!tgtBaseFound && !fallbackCUsed){
						wsprintf(InfoText,"IAT Rebuild:   All fallbacks exhausted for %s. DLL version differs significantly from dump source.", targetDllName);
						OutDebug(hWnd,InfoText);
						UnmapViewOfFile(tgtBase);
						CloseHandle(hTgtMapping);
						CloseHandle(hTgtFile);
						continue;
					}

					if(fallbackCUsed){
						// Fallback C resolved thunks directly, skip normal base-dependent resolution
						UnmapViewOfFile(tgtBase);
						CloseHandle(hTgtMapping);
						CloseHandle(hTgtFile);
						continue;
					}

					wsprintf(InfoText,"IAT Rebuild: Found %s base, resolving forwarded functions", targetDllName);
					OutDebug(hWnd,InfoText);

					// Resolve unresolved thunks via forwarded exports
					// For each unresolved thunk, find its RVA in the target DLL,
					// then look up the original function name from the importing DLL
					if(LoadedPe==TRUE){
						PIMAGE_THUNK_DATA32 rThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
						while((BYTE*)rThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
							if(rThk->u1.AddressOfData == 0) break;
							DWORD rVal = rThk->u1.AddressOfData;
							if(rVal <= SizeOfImage){ rThk++; continue; } // already resolved

							DWORD tgtRVAMatch = (DWORD)(rVal - tgtCandidateBase);
							// Find the target function name
							char *tgtFuncName = NULL;
							for(DWORD tn=0; tn<tgtExpDir->NumberOfNames; tn++){
								DWORD tIdx = tgtOrds[tn];
								if(tIdx < tgtExpDir->NumberOfFunctions && tgtFuncs[tIdx] == tgtRVAMatch){
									tgtFuncName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[tn]));
									break;
								}
							}

							wsprintf(InfoText,"IAT Rebuild:   thunk %08X -> tgtRVA=%08X, tgtFuncName=%s",
								rVal, tgtRVAMatch, tgtFuncName ? tgtFuncName : "(NULL)");
							OutDebug(hWnd,InfoText);

							// Find the original name from the importing DLL's forwarded exports
							char *originalName = NULL;
							if(tgtFuncName){
								int fwdCount = 0;
								for(DWORD oe=0; oe<dllExportDir->NumberOfFunctions; oe++){
									DWORD oeRVA = dllFunctions[oe];
									if(oeRVA < exportDirRVA || oeRVA >= exportDirRVA + exportDirSize) continue;
									fwdCount++;
									char *fwd = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, oeRVA));
									// Check if forwarder ends with ".FuncName"
									char *fwdDot = NULL;
									for(int fi=0; fwd[fi]!=0; fi++){ if(fwd[fi]=='.') fwdDot = &fwd[fi+1]; }
									if(fwdDot && _stricmp(fwdDot, tgtFuncName)==0){
										// Find the original export name for index oe
										for(DWORD on=0; on<dllExportDir->NumberOfNames; on++){
											if(dllOrdinals[on] == oe){
												originalName = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllNames[on]));
												break;
											}
										}
										if(originalName) break;
									}
								}
								if(!originalName){
									wsprintf(InfoText,"IAT Rebuild:   No forwarder match for '%s' in %s (%d forwarders scanned)", tgtFuncName, normalizedDll, fwdCount);
									OutDebug(hWnd,InfoText);
								}
							}

							if(originalName){
								DWORD nameLen = 0;
								while(originalName[nameLen] != 0) nameLen++;
								DWORD entrySize = (sizeof(WORD) + nameLen + 1 + 3) & ~3;
								if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
									*(WORD*)(pfile + writeOffset) = 0;
									for(DWORD c=0; c<=nameLen; c++)
										pfile[writeOffset + sizeof(WORD) + c] = originalName[c];
									rThk->u1.AddressOfData = writeOffset;
									wsprintf(InfoText,"  %s!%s (via %s)", normalizedDll, originalName, targetDllName);
									OutDebug(hWnd,InfoText);
									totalResolved++;
									unresolvedLeft--;
									writeOffset += entrySize;
								}
							}
							rThk++;
						}
					}
					else if(LoadedPe64==TRUE){
						PIMAGE_THUNK_DATA64 rThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
						while((BYTE*)rThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
							if(rThk64->u1.AddressOfData == 0) break;
							if(rThk64->u1.AddressOfData <= SizeOfImage){ rThk64++; continue; }

							DWORD tgtRVAMatch = (DWORD)(rThk64->u1.AddressOfData - tgtCandidateBase);
							char *tgtFuncName = NULL;
							for(DWORD tn=0; tn<tgtExpDir->NumberOfNames; tn++){
								DWORD tIdx = tgtOrds[tn];
								if(tIdx < tgtExpDir->NumberOfFunctions && tgtFuncs[tIdx] == tgtRVAMatch){
									tgtFuncName = (char*)(tgtBase + DllRvaToFileOffset(tgtBase, tgtNames[tn]));
									break;
								}
							}

							char *originalName = NULL;
							if(tgtFuncName){
								for(DWORD oe=0; oe<dllExportDir->NumberOfFunctions; oe++){
									DWORD oeRVA = dllFunctions[oe];
									if(oeRVA < exportDirRVA || oeRVA >= exportDirRVA + exportDirSize) continue;
									char *fwd = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, oeRVA));
									char *fwdDot = NULL;
									for(int fi=0; fwd[fi]!=0; fi++){ if(fwd[fi]=='.') fwdDot = &fwd[fi+1]; }
									if(fwdDot && strcmp(fwdDot, tgtFuncName)==0){
										for(DWORD on=0; on<dllExportDir->NumberOfNames; on++){
											if(dllOrdinals[on] == oe){
												originalName = (char*)(dllFileBase + DllRvaToFileOffset(dllFileBase, dllNames[on]));
												break;
											}
										}
										if(originalName) break;
									}
								}
							}

							if(originalName){
								DWORD nameLen = 0;
								while(originalName[nameLen] != 0) nameLen++;
								DWORD entrySize = (sizeof(WORD) + nameLen + 1 + 3) & ~3;
								if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
									*(WORD*)(pfile + writeOffset) = 0;
									for(DWORD c=0; c<=nameLen; c++)
										pfile[writeOffset + sizeof(WORD) + c] = originalName[c];
									rThk64->u1.AddressOfData = (ULONGLONG)writeOffset;
									wsprintf(InfoText,"  %s!%s (via %s)", normalizedDll, originalName, targetDllName);
									OutDebug(hWnd,InfoText);
									totalResolved++;
									unresolvedLeft--;
									writeOffset += entrySize;
								}
							}
							rThk64++;
						}
					}

					UnmapViewOfFile(tgtBase);
					CloseHandle(hTgtMapping);
					CloseHandle(hTgtFile);
				} // end for each export (forwarding scan)
			} // end if unresolvedLeft > 0
		} // end Phase 4b

		// Write placeholder entries for remaining unresolved thunks
		// This prevents ShowImports from crashing on invalid thunk values
		// For packer-suspect imports: zero out instead of writing placeholders
		if(isPacker){
			int packerZeroed = 0;
			if(LoadedPe==TRUE){
				PIMAGE_THUNK_DATA32 finalThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
				while((BYTE*)finalThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
					if(finalThk->u1.AddressOfData == 0) break;
					if(finalThk->u1.AddressOfData > SizeOfImage){
						finalThk->u1.AddressOfData = 0;
						packerZeroed++;
					}
					finalThk++;
				}
			}
			else if(LoadedPe64==TRUE){
				PIMAGE_THUNK_DATA64 finalThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
				while((BYTE*)finalThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
					if(finalThk64->u1.AddressOfData == 0) break;
					if(finalThk64->u1.AddressOfData > SizeOfImage){
						finalThk64->u1.AddressOfData = 0;
						packerZeroed++;
					}
					finalThk64++;
				}
			}
			if(packerZeroed > 0){
				wsprintf(InfoText,"  Packer import: %d thunks unresolved, zeroed (non-essential)", packerZeroed);
				OutDebug(hWnd,InfoText);
			}
		}
		else{
			if(LoadedPe==TRUE){
				PIMAGE_THUNK_DATA32 finalThk = (PIMAGE_THUNK_DATA32)(ftRVA + (DWORD)pfile);
				while((BYTE*)finalThk < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA32)){
					if(finalThk->u1.AddressOfData == 0) break;
					if(finalThk->u1.AddressOfData > SizeOfImage){
						char placeholder[64];
						wsprintf(placeholder, "Unresolved_%08X", finalThk->u1.AddressOfData);
						DWORD nameLen = 0;
						while(placeholder[nameLen] != 0) nameLen++;
						DWORD entrySize = (sizeof(WORD) + nameLen + 1 + 3) & ~3;
						if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
							*(WORD*)(pfile + writeOffset) = 0; // hint = 0
							for(DWORD c=0; c<=nameLen; c++)
								pfile[writeOffset + sizeof(WORD) + c] = placeholder[c];
							wsprintf(InfoText,"  %s!%s (unresolved)", normalizedDll, placeholder);
							OutDebug(hWnd,InfoText);
							finalThk->u1.AddressOfData = writeOffset;
							writeOffset += entrySize;
						}
						totalUnresolved++;
					}
					finalThk++;
				}
			}
			else if(LoadedPe64==TRUE){
				PIMAGE_THUNK_DATA64 finalThk64 = (PIMAGE_THUNK_DATA64)(ftRVA + (DWORD)pfile);
				while((BYTE*)finalThk64 < (BYTE*)pfile + hFileSize - sizeof(IMAGE_THUNK_DATA64)){
					if(finalThk64->u1.AddressOfData == 0) break;
					if(finalThk64->u1.AddressOfData > SizeOfImage){
						char placeholder[64];
						wsprintf(placeholder, "Unresolved_%I64X", finalThk64->u1.AddressOfData);
						DWORD nameLen = 0;
						while(placeholder[nameLen] != 0) nameLen++;
						DWORD entrySize = (sizeof(WORD) + nameLen + 1 + 3) & ~3;
						if(writeOffset + entrySize <= spaceEnd && writeOffset + entrySize <= (DWORD)hFileSize){
							*(WORD*)(pfile + writeOffset) = 0;
							for(DWORD c=0; c<=nameLen; c++)
								pfile[writeOffset + sizeof(WORD) + c] = placeholder[c];
							wsprintf(InfoText,"  %s!%s (unresolved)", normalizedDll, placeholder);
							OutDebug(hWnd,InfoText);
							finalThk64->u1.AddressOfData = (ULONGLONG)writeOffset;
							writeOffset += entrySize;
						}
						totalUnresolved++;
					}
					finalThk64++;
				}
			}
		}

		UnmapViewOfFile(dllFileBase);
		CloseHandle(hDllMapping);
		CloseHandle(hDllFile);
		curDesc++;
	}

	} // end two-pass for loop

	// =====================================================
	// Phase 5: Log results
	// =====================================================
	OutDebug(hWnd,"");
	wsprintf(InfoText,"IAT Rebuild: Processed %d DLLs (%d packer-suspect)", dllCount, packerSuspectCount);
	OutDebug(hWnd,InfoText);
	wsprintf(InfoText,"IAT Rebuild: Resolved %d functions, %d unresolved", totalResolved, totalUnresolved);
	OutDebug(hWnd,InfoText);
	if(writeOffset > freeSpaceOffset){
		wsprintf(InfoText,"IAT Rebuild: Used %d bytes for hint/name entries", writeOffset - freeSpaceOffset);
		OutDebug(hWnd,InfoText);
	}
	if(totalUnresolved > 0){
		OutDebug(hWnd,"IAT Rebuild: NOTE - Unresolved thunks are forwarded exports whose target");
		OutDebug(hWnd,"  DLLs on disk differ from the dump source. Use ImpREC on the live process");
		OutDebug(hWnd,"  or provide matching DLL files to resolve remaining imports.");
	}

	// =====================================================
	// Phase 6: Scan for unreferenced IAT groups
	// =====================================================
	if(packerSuspectCount > 0 && packerSuspectCount == importScanCount){
		OutDebug(hWnd,"IAT Rebuild: All imports are packer-suspect. Scanning for real IAT...");
		RebuildIAT_ScanUnreferenced(hWnd, &writeOffset, spaceEnd,
			importScan, importScanCount, importDesc);
	}
}

// ========================================================
// ============ Show the EXE Imported Functions ===========
// ========================================================
void ShowImports(HWND hWnd)
{
	/* 
	   Function:
	   ---------
	   "ShowImports"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler to update

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   The Function is Showing the imported functions that the
	   EXE is using in order to run the image and use system functions.
	   We Do it by scanning the import table and determining
	   The Loaded DLL ( at the Import descriptor), 
	   Than scan the fucntion's table ( uses Pointers ).
	   By reading each DWORD pointer ( point to the name of the function )
	   We finish the scanning when we read a NULL pointers (0x0000)
	   ����� ���� ��������� �������� �"� �����
	   ��� ���� �"� ����� ���� ������ ������ ��� �������
	   ��� DLL �� ����� ����� 0 �� ������ � ���� 
       �� ���� ���� 0 DLL �������� ����� �� ���� �������� �� �

       The Function returns Noting!
    */

	PIMAGE_IMPORT_DESCRIPTOR importDesc;	// Pointer to the Descriptor table
	PIMAGE_SECTION_HEADER pSection{};			// Pointer to the Sections
    PIMAGE_THUNK_DATA thunk, thunkIAT=0;	// Thunk pointer [pointers to functions]
	PIMAGE_THUNK_DATA64 thunk64, thunkIAT64=0;	// Thunk pointer [pointers to functions]
    PIMAGE_IMPORT_BY_NAME pOrdinalName;		// DLL imported name    
	HIMAGELIST hImageList;					// Pics on the Tree view are inside an ImageList
	HTREEITEM Parent,Before;				// Tree item level
	TV_INSERTSTRUCT tvinsert;				// Item's Properties
	HBITMAP hBitMap; 
	DWORD importsStartRVA{}, delta = -1;			// Hold Starting point of the RVA
	HWND ImpTree;							// Tree dialog hander
	DWORD len;								// Function Length
	bool error;
    char *pfile;							// Pointer to the memory mapped file
	char Import[256],Import2[128],Hint[100];
	
	// copy pointer to the memory mapped file
	pfile=FileMappedData;
	
	// get the handler to the tree control
	ImpTree=GetDlgItem(hWnd,IDC_IMPORTS_TREE);

	// attach images to the tree
	hImageList=ImageList_Create(16,16,ILC_COLOR16,5,10);
	hBitMap=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_IMPORTS_TREE));
	ImageList_Add(hImageList,hBitMap,NULL);

	// Kill object
	DeleteObject(hBitMap);

	// set the picture list
	SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_SETIMAGELIST,0,(LPARAM)hImageList);
	
	// get RVA of the import directory ( Extract the value of VirtualAddress )
	if(LoadedPe==TRUE) {
		importsStartRVA=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;	
	}
	else if(LoadedPe64==TRUE) {
		importsStartRVA=nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	}
	
	// RVA == 0 ??
	if(!importsStartRVA)
	{
		tvinsert.hParent=NULL;			// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvinsert.item.pszText="No Imports";
		tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);	
		return;
	}

	// Return the offset at file 
	if(LoadedPe==TRUE)
	{
		pSection=RVAToOffset(nt_header,section_header,importsStartRVA,&error);	
	}
	else if(LoadedPe64==TRUE)
	{
		pSection=RVAToOffset64(nt_header64,section_header,importsStartRVA,&error);
	}
	
	
	// No section ?
	if (!pSection)
	{
		// Check if the import directory is in the PE header area (e.g. after raw IAT reconstruction)
		// In the header area, file offset == RVA, so delta = 0
		DWORD sizeOfHeaders = LoadedPe ? nt_header->OptionalHeader.SizeOfHeaders
		                               : nt_header64->OptionalHeader.SizeOfHeaders;
		if(importsStartRVA < sizeOfHeaders && importsStartRVA + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= (DWORD)hFileSize){
			delta = 0; // Header area: file offset == RVA
		} else {
			tvinsert.hParent=NULL;			// top most level no need handle
			tvinsert.hInsertAfter=TVI_ROOT; // work as root level
			tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
			tvinsert.item.pszText="No Imports";
			tvinsert.item.iImage=0;
			tvinsert.item.iSelectedImage=1;
			SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
			return;
		}
	}
	else
	{
		// VA-Pointer to the offset
		delta = (INT)(pSection->VirtualAddress - pSection->PointerToRawData);
	}
	
	// (Use that value to go to the first IMAGE_IMPORT_DESCRIPTOR structure )
	// Get the address of DLL pointers
	importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (importsStartRVA - delta + (DWORD)pfile);

    if (importDesc==0 || (BYTE*)importDesc < (BYTE*)pfile || (BYTE*)importDesc >= (BYTE*)pfile + hFileSize) {
		tvinsert.hParent=NULL;			// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvinsert.item.pszText="Can't Get Imports";
		tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		return;
	}

	// read forever or untill 0 (No more imports)
	while(TRUE)
	{
		__try
		{
		// check if there are no more DLLs
		if ((importDesc->TimeDateStamp==0) && (importDesc->Name==0))
            break;  // finished reading the dlls
		}
		__except (MessageBox(hWnd,"Can't get DLLs","PVDasm Handler",MB_OK))
		{
			ClearIMpex(ImpTree,"Currupted Import Directory");
			return;
		}
		
		// DLL name
		// I used exception handler to avoid reading invalid
		// Memory address
		__try
		{
		  wsprintf(Import,"%s", (PBYTE)(importDesc->Name) - delta + (DWORD)pfile);
		}
		__except (MessageBox(hWnd,"Can't get Import Name!","PVDasm Handler",MB_OK))
		{
			ClearIMpex(ImpTree,"Currupted Import Directory");
			return;
		}
		// insert name to the tree (parent)
		tvinsert.hParent=NULL;			// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvinsert.item.pszText=Import;
		tvinsert.item.iImage=2;
		tvinsert.item.iSelectedImage=2;
		Before=Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		// insert the DLL information under the imported name at the tree control
		wsprintf(Import2," OriginalFirstThunk: %08X", importDesc->OriginalFirstThunk);
		tvinsert.hParent=Parent;
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.pszText=Import2;
        tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
        SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		
		wsprintf(Import2," TimeDateStamp:   %08X", importDesc->TimeDateStamp);
		tvinsert.hParent=Parent;
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.pszText=Import2;
        tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

		wsprintf(Import2," ForwarderChain:  %08X", importDesc->ForwarderChain);
		tvinsert.hParent=Parent;		// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.pszText=Import2;
        tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

		wsprintf(Import2," First thunk RVA: %08X", importDesc->FirstThunk);
		tvinsert.hParent=Parent;		// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.pszText=Import2;
        tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		
		// lets the get the imports
        tvinsert.item.iImage=3;
		tvinsert.item.iSelectedImage=3;
		tvinsert.item.pszText="Imported Functions";
		Before=Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		
		// read thunks (pointers to the functions)
		if(LoadedPe==TRUE)
		{
			thunk = (PIMAGE_THUNK_DATA)importDesc->Characteristics;	// OriginalFirstThunk
			thunkIAT = (PIMAGE_THUNK_DATA)importDesc->FirstThunk;	// FirstThunk
		}
		else if(LoadedPe64==TRUE)
		{
			thunk64 = (PIMAGE_THUNK_DATA64)importDesc->Characteristics;	// OriginalFirstThunk
			thunkIAT64 = (PIMAGE_THUNK_DATA64)importDesc->FirstThunk;	// FirstThunk
		}

		if(LoadedPe==TRUE)
		{
			// If OriginalFirstThunk is zero, use the value in FirstThunk instead
			if(thunk==0)	// No Characteristics field?
			{
				// Yes! Gotta have a non-zero FirstThunk field then.
				thunk = thunkIAT;
            
				if ( thunk == 0 )
				{   // No FirstThunk field?  Ooops!!!
					MessageBox(hWnd,"No FirstThunk...","PVDasm Notice",MB_OK|MB_ICONINFORMATION);
					return ;
				}
             
			}
		}
		else if(LoadedPe64==TRUE)
		{
			// If OriginalFirstThunk is zero, use the value in FirstThunk instead
			if(thunk64==0)	// No Characteristics field?
			{
				// Yes! Gotta have a non-zero FirstThunk field then.
				thunk64 = thunkIAT64;
            
				if ( thunk64 == 0 )
				{   // No FirstThunk field?  Ooops!!!
					MessageBox(hWnd,"No FirstThunk...","PVDasm Notice",MB_OK|MB_ICONINFORMATION);
					return ;
				}
			}
		}

		// lets go to the Thunk table and get ready to strip the fucntions
		if(LoadedPe==TRUE)
		{
			thunk = (PIMAGE_THUNK_DATA)( (PBYTE)thunk - delta + (DWORD)pfile);
			thunkIAT = (PIMAGE_THUNK_DATA)( (PBYTE)thunkIAT - delta + (DWORD)pfile);
		}
		else if(LoadedPe64==TRUE)
		{
			thunk64 = (PIMAGE_THUNK_DATA64)( (PBYTE)thunk64 - delta + (DWORD)pfile);
			thunkIAT64 = (PIMAGE_THUNK_DATA64)( (PBYTE)thunkIAT64 - delta + (DWORD)pfile);
		}

		// while there are imports strip them and show them at the tree control
		while(TRUE)
		{
			// reset the content
			Hint[0]=NULL;
			Import[0]=NULL;
			Import2[0]=NULL;

			// Thunk is Zero?..finished reading the DLL's functions
			if(LoadedPe==TRUE)
			{
				if (thunk->u1.AddressOfData==0)
		            break;

			}
			else if(LoadedPe64==TRUE)
			{
				if (thunk64->u1.AddressOfData==0)
					break;
			}

			/*
			   For each member in the array, 
			   We check the value of the member against IMAGE_ORDINAL_FLAG32. 
			   If the most significant bit of the member is 1, 
			   Then the function is exported by ordinal and we can extract 
			   The ordinal number from the low word of the member. 
			   If the most significant bit of the member is 0, 
			   Use the value in the member as the RVA into,
			   The IMAGE_IMPORT_BY_NAME, skip Hint, and you're at the 
			   Name of the function. 
            */
	
			// If this Flag is Set than read Ordinal!
			if(LoadedPe==TRUE)
			{
				if (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
				{
					// print the ordianl
					__try{
					wsprintf(Hint,"Hint: %u ", IMAGE_ORDINAL(thunk->u1.Ordinal) );	
					wsprintf(Import,"%s","Ordinal");
					}
					// If exception accurd Trigger the User!
					__except (MessageBox(hWnd,"Can't get Imports!","PVDasm Handler",MB_OK))
					{
						ClearIMpex(ImpTree,"Currupted Import Directory");
						return;
					}
				}
				else // Else read funtion's Name & Hint & IAT address
				{
					// copy the ordinal and the imported function
					pOrdinalName = (PIMAGE_IMPORT_BY_NAME)thunk->u1.AddressOfData;
					pOrdinalName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)pOrdinalName - delta + (DWORD)pfile);    
					__try{
					wsprintf(Hint,"Hint: %04u", pOrdinalName->Hint);
					
					// This Code Prevents Buffer Overflow 
					// When using wsprintf with a function with more
					// Than 256 chars on 'Import[256]' buffer
					len=StringLen((char*)pOrdinalName->Name);
					if(len>256)
					{
						for(int i=0;i<256;i++)
							Import[i]=pOrdinalName->Name[i];
					}
					else
						wsprintf(Import,"%s",pOrdinalName->Name); // No overflow
					}
					// If exception accurd Trigger the User!
					__except (MessageBox(hWnd,"Can't get Imports!","PVDasm Handler",MB_OK))
					{
						ClearIMpex(ImpTree,"Currupted Import Directory");
						return;
					}
				}
			}
			else if(LoadedPe64==TRUE)
			{
				if (thunk64->u1.Ordinal & IMAGE_ORDINAL_FLAG64)
				{
					// print the ordianl
					__try{
					wsprintf(Hint,"Hint: %u ", IMAGE_ORDINAL64(thunk64->u1.Ordinal) );	
					wsprintf(Import,"%s","Ordinal");
					}
					// If exception accurd Trigger the User!
					__except (MessageBox(hWnd,"Can't get Imports!","PVDasm Handler",MB_OK))
					{
						ClearIMpex(ImpTree,"Currupted Import Directory");
						return;
					}
				}
				else // Else read funtion's Name & Hint & IAT address
				{
					// copy the ordinal and the imported function
					pOrdinalName = (PIMAGE_IMPORT_BY_NAME)thunk64->u1.AddressOfData;
					pOrdinalName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)pOrdinalName - delta + (DWORD)pfile);    
					__try{
					wsprintf(Hint,"Hint: %04u", pOrdinalName->Hint);
					
					// This Code Prevents Buffer Overflow 
					// When using wsprintf with a function with more
					// Than 256 chars on 'Import[256]' buffer
					len=StringLen((char*)pOrdinalName->Name);
					if(len>256)
					{
						for(int i=0;i<256;i++)
							Import[i]=pOrdinalName->Name[i];
					}
					else
						wsprintf(Import,"%s",pOrdinalName->Name); // No overflow
					}
					// If exception accurd Trigger the User!
					__except (MessageBox(hWnd,"Can't get Imports!","PVDasm Handler",MB_OK))
					{
						ClearIMpex(ImpTree,"Currupted Import Directory");
						return;
					}
				}
			}
            
			if(LoadedPe==TRUE)
			{
				// Save import address table
				__try{
				wsprintf(Import2, "IAT: %08X", thunkIAT->u1.Function );
				}
				// If exception accurd Trigger the User!
				__except (MessageBox(hWnd,"Can't get Imports!","PVDasm Handler",MB_OK))
				{
					ClearIMpex(ImpTree,"Currupted Import Directory");
					return;
				}
			}
			else if(LoadedPe64==TRUE)
			{
				// Save import address table
				__try{
				wsprintf(Import2, "IAT: %08X", thunkIAT64->u1.Function );
				}
				// If exception accurd Trigger the User!
				__except (MessageBox(hWnd,"Can't get Imports!","PVDasm Handler",MB_OK))
				{
					ClearIMpex(ImpTree,"Currupted Import Directory");
					return;
				}
			}
			

			// Shwing the information on the tree view
			tvinsert.hParent=Parent;
			tvinsert.hInsertAfter=TVI_LAST;
			tvinsert.item.pszText=Import;
            tvinsert.item.iImage=4;
		    tvinsert.item.iSelectedImage=4;
			Before=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		    
            tvinsert.item.iImage=0;
		    tvinsert.item.iSelectedImage=1;
			tvinsert.hParent=Before;
			tvinsert.item.pszText=Hint;
			SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
			
			tvinsert.hParent=Before;
			tvinsert.item.pszText=Import2;
			SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
			
			/*
			   Skip to the next array member, and retrieve the names 
			   Until the end of the array is reached (it's null -terminated). 
			   Now we are done extracting the names of the functions imported from a DLL. 
			   We go to the next DLL. 
			*/

			if(LoadedPe==TRUE)
			{
				thunk++;		// Read next thunk
				thunkIAT++;		// Read next IAT thunk
			}
			else if(LoadedPe64==TRUE)
			{
				thunk64++;		// Read next thunk
				thunkIAT64++;		// Read next IAT thunk
			}
		}
		
		/*
		   Do that until the end of the array is reached (IMAGE_IMPORT_DESCRIPTOR, 
		   Array is terminated by a member with all zeroes in its fields). 
		*/
		importDesc++;   // advance to next IMAGE_IMPORT_DESCRIPTOR
	}

}

// ========================================================
// ============ Show the EXE Imported Functions ===========
// ========================================================
void ShowExports(HWND hWnd)
{
	/* 
	   Function:
	   ---------
	   "ShowImports"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler to update

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   Showing the Exported functions (DLL) that the
	   EXE is using in order to run the image.
	   by scanning the Expotr table and determining
	   the DLL, than scan the fucntion's table
	   by reading each DWORD pointer (point to the name of the function)
    
	   ����� ���� ��������� �������� �"� �����
	   ��� ���� �"� ����� ���� ������ ������ ��� �������
	   DLL �� ����� ����� 0 �� ������ �
       �� ���� ���� 0 DLL �������� ����� �� ���� �������� �� �
    */
	
	// Local variables
	HWND ExpTree;
	PIMAGE_EXPORT_DIRECTORY exportDir;	// Export struct
	PIMAGE_SECTION_HEADER header{};		// Section struct
	DWORD i;
	PDWORD functions;
	PWORD ordinals;
	INT delta;
	DWORD exportsStartRVA, exportsEndRVA;
	DWORD base,NumberOfFunctions,NumberOfNames;
	HIMAGELIST hImageList;
	HTREEITEM Parent,Before;
	TV_INSERTSTRUCT tvinsert;
	HBITMAP hBitMap;
	bool error;
    char *filename;
	char *pfile,line[100],Function[100],Entry[100],Ordinal[100];
	LPSTR *name;

	// Memory mapped file pointer copy
	pfile=FileMappedData;
	// address of mapped file pointer (dword cast)
	base = (DWORD)pfile;
	if(base==0 || pfile==0 )
	{
		MessageBox(hWnd,"Can't Get Exports","PVDasm Notice",MB_OK|MB_ICONINFORMATION);
		return;
	}
	// get the handle of the export tree window
	ExpTree=GetDlgItem(hWnd,IDC_EXPORTS_TREE);

	// attach images to the tree
	hImageList=ImageList_Create(16,16,ILC_COLOR16,5,10);
	hBitMap=LoadBitmap(hInstance,MAKEINTRESOURCE(IDB_EXPORTS_TREE));
	ImageList_Add(hImageList,hBitMap,NULL);
	// Kill object
	DeleteObject(hBitMap);
	// set the picture list
	SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_SETIMAGELIST,0,(LPARAM)hImageList);
    
	// get RVA of export directory (usualy the first in the array)
    if(LoadedPe==TRUE)
	{
		exportsStartRVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;	
	}
	else if(LoadedPe64==TRUE)
	{
		exportsStartRVA = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	}

	if(!exportsStartRVA)
	{
		// check if there is a section with the exports
		tvinsert.hParent=NULL;			// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvinsert.item.pszText="No Exports Table";
		tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		return;
	}
    
	// calculate the size of the export table
	if(LoadedPe==TRUE)
	{
		exportsEndRVA = exportsStartRVA + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}
	else if(LoadedPe64==TRUE)
	{
		exportsEndRVA = exportsStartRVA + nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}
	
	// get the section of exports (usualy the .idata section)
	if(LoadedPe==TRUE)
	{
		header=RVAToOffset(nt_header,section_header,exportsStartRVA,&error);
	}
	else if(LoadedPe64==TRUE)
	{
		header=RVAToOffset64(nt_header64,section_header,exportsStartRVA,&error);
	}
	
	// check if there is a section with the exports
	if (!header)
	{
		tvinsert.hParent=NULL;			// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvinsert.item.pszText="No Exports Table";
		tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		return;
	}

	delta = (INT)(header->VirtualAddress - header->PointerToRawData);

	// pointer to the expotr directory
	exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base,exportsStartRVA - delta);
	
	if ((BYTE*)exportDir < (BYTE*)pfile || (BYTE*)exportDir >(BYTE*)pfile + nt_header->OptionalHeader.SizeOfImage) {
		ClearIMpex(ExpTree, "Currupted Export Directory");
		return;
	}


	// get the pointer to the exported dll name
	__try{
		filename = (PSTR)(exportDir->Name - delta + base);
		wsprintf(line,"%s", filename);
	}
	__except (MessageBox(hWnd,"Can't get Export Name!","PVDasm Handler",MB_OK))
	{
		ClearIMpex(ExpTree,"Currupted Export Directory");
		return;
	}
	// show the dll name at the tree item
	tvinsert.hParent=NULL;			// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvinsert.item.pszText=line;
	tvinsert.item.iImage=2;
	tvinsert.item.iSelectedImage=2;
	Before=Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
	
	// show the dll information
	wsprintf(line,"Characteristics: %08X", exportDir->Characteristics);
	tvinsert.hParent=Parent;		// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.pszText=line;
    tvinsert.item.iImage=0;
	tvinsert.item.iSelectedImage=1;
	SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
	// created time
	wsprintf(line,"TimeDateStamp: %08X", exportDir->TimeDateStamp);	
	tvinsert.hParent=Parent;		// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.pszText=line;
	SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
	// dll version vX.XX
	wsprintf(line,"Version: %u.%02u", exportDir->MajorVersion,exportDir->MinorVersion);
	tvinsert.hParent=Parent;		// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.pszText=line;
	SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
	// get ordinal
	wsprintf(line,"Ordinal base: %08X", exportDir->Base);
	tvinsert.hParent=Parent;		// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.pszText=line;
	SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
    // number of exported functions
	wsprintf(line,"Number of functions: %08X", exportDir->NumberOfFunctions);
	tvinsert.hParent=Parent;		// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.pszText=line;
	SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
    // number of functions
	wsprintf(line,"Number of Names: %08X", exportDir->NumberOfNames);
	tvinsert.hParent=Parent;		// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.pszText=line;
	SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

	// address of exported functions
	functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
	if(functions==0 )
	{
		return;
	}
	
    // address of exported ordinals
    ordinals = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
	if(ordinals==0 )
	{
		return;
	}
	// get address of names
    name = (PSTR *)((DWORD)exportDir->AddressOfNames - delta + base);

	tvinsert.hParent=Parent;		// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.pszText="Exported Functions";
    tvinsert.item.iImage=3;
	tvinsert.item.iSelectedImage=3;
	Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

	// scan the exported functions
	NumberOfFunctions=exportDir->NumberOfFunctions;
	for (i=0;i<NumberOfFunctions;i++)
    {
		// entry point of each function
        DWORD entryPointRVA = functions[i];
        DWORD j;

		// Put NULL terminator at beginning 
		Function[0]=NULL;
		Entry[0]=NULL;
		Ordinal[0]=NULL;

        if (entryPointRVA==0)	// Skip over gaps in exported function
            continue;			// ordinals (the entrypoint is 0 for
								// these functions).
        wsprintf(Entry,"Entry Point: %08X", entryPointRVA);
		wsprintf(Ordinal,"Ordinal: %4u", i + exportDir->Base );

        // See if this function has an associated name exported for it.
		NumberOfNames=exportDir->NumberOfNames;
        for (j=0;j<NumberOfNames;j++)
		{
            if (ordinals[j]==i)
			{
				__try{
                wsprintf(Function,"%s ",name[j]-delta+base);
				}
				// Allert the User!
				__except (MessageBox(hWnd,"Can't Get Exported Function!","PVDasm Handler",MB_OK))
				{
					ClearIMpex(ExpTree,"Currupted Export Directory");
					return;
				}
			}

		}

        // Is it a forwarder?  If so, the entry point RVA is inside the
        // .edata section, and is an RVA to the DllName.EntryPointName
        if ( (entryPointRVA >= exportsStartRVA)
             && (entryPointRVA <= exportsEndRVA) )
		{
			__try{
            wsprintf(Function,"(forwarder -> %s)", entryPointRVA - delta + base );
			}
			// Allert the User!
			__except (MessageBox(hWnd,"Can't Get Function Forwarder!","PVDasm Handler",MB_OK))
			{
				ClearIMpex(ExpTree,"Currupted Export Directory");
				return;
			}
        }

		// show the function
		tvinsert.hParent=Parent;
		tvinsert.hInsertAfter=TVI_ROOT;
		tvinsert.item.pszText=Function;
        tvinsert.item.iImage=4;
	    tvinsert.item.iSelectedImage=4;
        Before=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);   
        // show entry point of the function
		tvinsert.hParent=Before;	// top most level no need handle
		tvinsert.item.pszText=Entry;
        tvinsert.item.iImage=0;
	    tvinsert.item.iSelectedImage=1;
        SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);   
		// show ordinal in tree view
		tvinsert.hParent=Before;	// top most level no need handle
		tvinsert.item.pszText=Ordinal;
        SendDlgItemMessage(hWnd,IDC_EXPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);    
	}
}

void ClearIMpex(HWND hWnd,char *msg)
{
	/*
	   Function:
	   ---------
	   "ClearIMpex"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler to update
	   2. char *msg - pointer to char/string to show

	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   We call this function
	   Only when an Exception accurd in our
	   Imports/Expotrs functions.
	   the function will clear the Tree of the hWnd specified
	   And will put a new message (char *msg);
    */

	TV_INSERTSTRUCT tvinsert;

	// Clear all items
	TreeView_DeleteAllItems(hWnd);
	// put message
	tvinsert.hParent=NULL;			// top most level no need handle
	tvinsert.hInsertAfter=TVI_ROOT; // work as root level
	tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvinsert.item.pszText=msg;
	tvinsert.item.iImage=0;
	tvinsert.item.iSelectedImage=1;
	// put text on tree view
	SendMessage(hWnd,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
}

DWORD GetNumberOfExports(char* FilePtr)
{
	PIMAGE_EXPORT_DIRECTORY exportDir;	// Export struct
	PIMAGE_SECTION_HEADER header;		// Section struct
	IMAGE_DOS_HEADER* dos_header;
	IMAGE_NT_HEADERS* nt_header;
	IMAGE_NT_HEADERS64* nt_header64;
	DWORD base;
	DWORD exportsStartRVA, exportsEndRVA;
	INT delta;
	bool error;
	char *pfile;
	
	// Memory mapped file pointer copy
	pfile=FilePtr;
	dos_header=(IMAGE_DOS_HEADER*)pfile;
	if(LoadedPe==TRUE){
		nt_header=(IMAGE_NT_HEADERS*)(dos_header->e_lfanew+pfile);
	}
	else if(LoadedPe64==TRUE){
		nt_header64=(IMAGE_NT_HEADERS64*)(dos_header->e_lfanew+pfile);
	}
	base = (DWORD)pfile;
	if(base==0 || pfile==0 ){
		return 0;
	}

	if(LoadedPe==TRUE){
		exportsStartRVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;	
	}
	else if(LoadedPe64==TRUE){
		exportsStartRVA = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	}

	// calculate the size of the export table
	if(LoadedPe==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}
	else if(LoadedPe64==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}

	if(!exportsStartRVA){
		return 0;
	}

	// get the section of exports (usualy the .idata section)
	if(LoadedPe==TRUE){
		header=RVAToOffset(nt_header,section_header,exportsStartRVA,&error);
	}
	else if(LoadedPe64==TRUE){
		header=RVAToOffset64(nt_header64,section_header,exportsStartRVA,&error);
	}

	if(!header){
		return 0;
	}

	delta = (INT)(header->VirtualAddress - header->PointerToRawData);
	
	// pointer to the expotr directory
	exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base,exportsStartRVA - delta);

	return exportDir->NumberOfFunctions;
}

void GetExportName(char* FilePtr, DWORD Index, char* ExportName){
	PIMAGE_EXPORT_DIRECTORY exportDir=NULL;	// Export struct
	PIMAGE_SECTION_HEADER header=NULL;		// Section struct
	IMAGE_DOS_HEADER* dos_header=NULL;
	IMAGE_NT_HEADERS* nt_header=NULL;
	IMAGE_NT_HEADERS64* nt_header64=NULL;
	DWORD base=NULL;
	DWORD exportsStartRVA=NULL, exportsEndRVA=NULL,NumberOfNames=NULL;
	PDWORD functions=NULL;
	PWORD ordinals=NULL;
	INT delta=NULL;
	bool error;
	char *pfile;
	LPSTR *name;
	
	// Memory mapped file pointer copy
	pfile=FilePtr;
	dos_header=(IMAGE_DOS_HEADER*)pfile;
	if(LoadedPe==TRUE){
		nt_header=(IMAGE_NT_HEADERS*)(dos_header->e_lfanew+pfile);
	}
	else if(LoadedPe64==TRUE){
		nt_header64=(IMAGE_NT_HEADERS64*)(dos_header->e_lfanew+pfile);
	}
	base = (DWORD)pfile;
	if(base==0 || pfile==0 ){
		return;
	}

	if(LoadedPe==TRUE){
		exportsStartRVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;	
	}
	else if(LoadedPe64==TRUE){
		exportsStartRVA = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	}

	// calculate the size of the export table
	if(LoadedPe==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}
	else if(LoadedPe64==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}

	if(!exportsStartRVA){
		return;
	}

	// get the section of exports (usualy the .idata section)
	if(LoadedPe==TRUE){
		header=RVAToOffset(nt_header,section_header,exportsStartRVA,&error);
	}
	else if(LoadedPe64==TRUE){
		header=RVAToOffset64(nt_header64,section_header,exportsStartRVA,&error);
	}

	if(!header){
		return;
	}

	delta = (INT)(header->VirtualAddress - header->PointerToRawData);
	
	// pointer to the expotr directory
	exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base,exportsStartRVA - delta);
	
	// address of exported functions
	functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
	if(functions==0){
		return;
	}

	// address of exported ordinals
    ordinals = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
	if(ordinals==0){
		return;
	}

	// get address of names
    name = (PSTR *)((DWORD)exportDir->AddressOfNames - delta + base);

	// check if Index is out of bound
	if(Index>exportDir->NumberOfFunctions || Index<0)
		return;

	NumberOfNames=exportDir->NumberOfNames;
	ExportName[0]=NULL;
	for (DWORD j=0;j<NumberOfNames;j++){
		if(ordinals[j]==Index){
			try{
               wsprintf(ExportName,"%s ",name[j]-delta+base);
			   break;
			}
			catch (...) {}
		}
	}
	
}

DWORD GetExportDisasmIndex(char* FilePtr, DWORD Index){
	PIMAGE_EXPORT_DIRECTORY exportDir=NULL;	// Export struct
	PIMAGE_SECTION_HEADER header=NULL;		// Section struct
	IMAGE_DOS_HEADER* dos_header=NULL;
	IMAGE_NT_HEADERS* nt_header=NULL;
	IMAGE_NT_HEADERS64* nt_header64=NULL;
	DWORD base=NULL;
	DWORD FunctionEntryPoint=-1;
	DWORD exportsStartRVA=NULL, exportsEndRVA=NULL,NumberOfNames=NULL;
	PDWORD functions=NULL;
	PWORD ordinals=NULL;
	INT delta=NULL;
	bool error;
	char *pfile;
	LPSTR *name;
	
	// Memory mapped file pointer copy
	pfile=FilePtr;
	dos_header=(IMAGE_DOS_HEADER*)pfile;
	if(LoadedPe==TRUE){
		nt_header=(IMAGE_NT_HEADERS*)(dos_header->e_lfanew+pfile);
	}
	else if(LoadedPe64==TRUE){
		nt_header64=(IMAGE_NT_HEADERS64*)(dos_header->e_lfanew+pfile);
	}
	base = (DWORD)pfile;
	if(base==0 || pfile==0 ){
		return -1;
	}

	if(LoadedPe==TRUE){
		exportsStartRVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;	
	}
	else if(LoadedPe64==TRUE){
		exportsStartRVA = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	}

	// calculate the size of the export table
	if(LoadedPe==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}
	else if(LoadedPe64==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}

	if(!exportsStartRVA){
		return -1;
	}

	// get the section of exports (usualy the .idata section)
	if(LoadedPe==TRUE){
		header=RVAToOffset(nt_header,section_header,exportsStartRVA,&error);
	}
	else if(LoadedPe64==TRUE){
		header=RVAToOffset64(nt_header64,section_header,exportsStartRVA,&error);
	}

	if(!header){
		return -1;
	}

	delta = (INT)(header->VirtualAddress - header->PointerToRawData);
	
	// pointer to the expotr directory
	exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base,exportsStartRVA - delta);
	
	// address of exported functions
	functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
	if(functions==0){
		return -1;
	}

	// address of exported ordinals
    ordinals = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
	if(ordinals==0){
		return -1;
	}

	// get address of names
    name = (PSTR *)((DWORD)exportDir->AddressOfNames - delta + base);

	// check if Index is out of bound
	if(Index>exportDir->NumberOfFunctions || Index<0)
		return -1;

	NumberOfNames=exportDir->NumberOfNames;
	for (DWORD j=0;j<NumberOfNames;j++){
		//if(ordinals[j]==Index){
			try{
				if(LoadedPe==TRUE){
					FunctionEntryPoint=nt_header->OptionalHeader.ImageBase+functions[Index];
				}
				else if(LoadedPe64==TRUE){
					FunctionEntryPoint=(DWORD)nt_header64->OptionalHeader.ImageBase+functions[Index];
				}
			   break;
			}
			catch (...) {return -1;}
	//	}
	}

	if(FunctionEntryPoint==-1){
		return -1;
	}

	DWORD DisasmSize=(DWORD)DisasmDataLines.size();
	for(DWORD DisasmIndex=0;DisasmIndex<DisasmSize;DisasmIndex++)
	{
		DWORD Address = StringToDword(DisasmDataLines[DisasmIndex].GetAddress());
		if(FunctionEntryPoint == Address){
			return DisasmIndex;
		}
	}
	return -1;
}

DWORD GetExportOrdinal(char* FilePtr, DWORD Index )
{
	PIMAGE_EXPORT_DIRECTORY exportDir=NULL;	// Export struct
	PIMAGE_SECTION_HEADER header=NULL;		// Section struct
	IMAGE_DOS_HEADER* dos_header=NULL;
	IMAGE_NT_HEADERS* nt_header=NULL;
	IMAGE_NT_HEADERS64* nt_header64=NULL;
	DWORD base=NULL,entryPointRVA=NULL;
	DWORD exportsStartRVA=NULL, exportsEndRVA=NULL;
	DWORD NumberOfFunctions=NULL;
	PDWORD functions=NULL;
	PWORD ordinals=NULL;
	INT delta=NULL;
	bool error;
	char *pfile;
	
	// Memory mapped file pointer copy
	pfile=FilePtr;
	dos_header=(IMAGE_DOS_HEADER*)pfile;
	if(LoadedPe==TRUE){
		nt_header=(IMAGE_NT_HEADERS*)(dos_header->e_lfanew+pfile);
	}
	else if(LoadedPe64==TRUE){
		nt_header64=(IMAGE_NT_HEADERS64*)(dos_header->e_lfanew+pfile);
	}
	base = (DWORD)pfile;
	if(base==0 || pfile==0 ){
		return -1;
	}

	if(LoadedPe==TRUE){
		exportsStartRVA = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;	
	}
	else if(LoadedPe64==TRUE){
		exportsStartRVA = nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	}

	// calculate the size of the export table
	if(LoadedPe==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}
	else if(LoadedPe64==TRUE){
		exportsEndRVA = exportsStartRVA + nt_header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
	}

	if(!exportsStartRVA){
		return -1;
	}

	// get the section of exports (usualy the .idata section)
	if(LoadedPe==TRUE){
		header=RVAToOffset(nt_header,section_header,exportsStartRVA,&error);
	}
	else if(LoadedPe64==TRUE){
		header=RVAToOffset64(nt_header64,section_header,exportsStartRVA,&error);
	}

	if(!header){
		return -1;
	}

	delta = (INT)(header->VirtualAddress - header->PointerToRawData);
	
	// pointer to the expotr directory
	exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base,exportsStartRVA - delta);
	
	// address of exported functions
	functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
	if(functions==0){
		return -1;
	}

	// address of exported ordinals
    ordinals = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
	if(ordinals==0){
		return -1;
	}

	// check if Index is out of bound
	if(Index>exportDir->NumberOfFunctions || Index<0)
		return -1;

	NumberOfFunctions=exportDir->NumberOfFunctions;
	for (DWORD j=0;j<NumberOfFunctions;j++){
								// these functions).
		if(j==Index){
			return j + exportDir->Base;
		}
	}

	return -1;
}