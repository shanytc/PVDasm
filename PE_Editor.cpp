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
	
	   פונקציה זו מקבלת 3 משתנים
	   הראשון הוא מצביע לקובץ הממופה בזיכרון כדי שנוכל
	   לגשת לראש הקובץ לקרוא אותו.
	   כמו כן הפונקציה מקבלת את המידע על
	   החלון הראשי וכמו כן את העצם המזהה את הקובץ

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

	   שמירת הנתונים שבתיבות הטקסט
	   לתוך מבנה ראש הקובץ

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

	   הפונקציה מציגה את הפרטים הבסיסיים של ראש הקובץ
	   על גבי תיבות הטקסט.

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

	   הפרוצדורה מעדכנת את איזור התיקיות של קובץ
	   הראש ע"י קריאת הנתונים מהדיאלוג ושמירתם בחזרה לזיכרון
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
	
	   פונקציה זו בונה מחדש את קובץ הראש על מנת
	   לגרום לו לרוץ על מערכת ההפעלה.
	   מכיוון ששמרנו אותו מן הזיכרון הקובץ השתנה
	   ואנו צריכים לבנות אותו
	   אנו עושים זאת ע"י שינוי כמה מהמאפיינים של הראש לברירת המחדל
	   של המהדר בכללי.
	*/

	DWORD i,NumberOfSections;
	DWORD temp=0x00001000; // aligment/size of 4096 (1000h)
	char InfoText[128];

	// print debug information
	OutDebug(hWnd,"");

	if(LoadedPe==TRUE){
		OutDebug(hWnd,"-> Starting PE Rebuilding Session <-");
		OutDebug(hWnd,"============================");
		wsprintf(InfoText,"Overiding File Alignment from %08X to %08X",nt_header->OptionalHeader.FileAlignment,temp); 
	}
	else if(LoadedPe64==TRUE)
	{
		OutDebug(hWnd,"-> Starting PE+ Rebuilding Session <-");	
		OutDebug(hWnd,"============================");
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
		nt_header->OptionalHeader.SizeOfHeaders=temp; // too big size
	}
	else if(LoadedPe64==TRUE)
	{
		nt_header64->OptionalHeader.SizeOfHeaders=temp; // too big size
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
	for(i=0;i<NumberOfSections;i++)
	{
		wsprintf(InfoText,"Overiding Section: %s -> Size Of Raw Data from %08X to %08X",section_header->Name,section_header->SizeOfRawData,section_header->Misc.VirtualSize); 
		section_header->SizeOfRawData			= section_header->Misc.VirtualSize;
		OutDebug(hWnd,InfoText);
		wsprintf(InfoText,"Overiding Section: %s -> Pointer To Raw Data from %08X to %08X",section_header->Name,section_header->PointerToRawData,section_header->VirtualAddress); 
		section_header->PointerToRawData		= section_header->VirtualAddress;	
		OutDebug(hWnd,InfoText);
		OutDebug(hWnd,"");
		// Next Section
		section_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(section_header)+sizeof(IMAGE_SECTION_HEADER));
	}
	
	// rebuild task finished
	OutDebug(hWnd,"-> Rebuild Task Completed..");
	OutDebug(hWnd,"====================");
	OutDebug(hWnd,"");
	MessageBox(hWnd,"Rebuilding Task Completed!","PE Repair",MB_OK|MB_ICONINFORMATION);
	
	// Exit
	return;
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
	   קריאת טבלת הפונקציות המשומשות ע"י הקובץ
	   זאת נעשה ע"י קריאת מילה שמשמשת כמצביע לשם הפוקציה
	   הבא DLL אם המילה מכילה 0 אז עוברים ל קובץ 
       עד שהוא מכיל 0 DLL וממשיכים לקרוא את טבלת המצביעים של ה

       The Function returns Noting!
    */

	PIMAGE_IMPORT_DESCRIPTOR importDesc;	// Pointer to the Descriptor table
    PIMAGE_SECTION_HEADER pSection;			// Pointer to the Sections
    PIMAGE_THUNK_DATA thunk, thunkIAT=0;	// Thunk pointer [pointers to functions]
	PIMAGE_THUNK_DATA64 thunk64, thunkIAT64=0;	// Thunk pointer [pointers to functions]
    PIMAGE_IMPORT_BY_NAME pOrdinalName;		// DLL imported name    
	HIMAGELIST hImageList;					// Pics on the Tree view are inside an ImageList
	HTREEITEM Parent,Before;				// Tree item level
	TV_INSERTSTRUCT tvinsert;				// Item's Properties
	HBITMAP hBitMap; 
	DWORD importsStartRVA,delta=-1;			// Hold Starting point of the RVA
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
	if(LoadedPe==TRUE)
	{
		importsStartRVA=nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;	
	}
	else if(LoadedPe64==TRUE)
	{
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
		tvinsert.hParent=NULL;			// top most level no need handle
		tvinsert.hInsertAfter=TVI_ROOT; // work as root level
		tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		tvinsert.item.pszText="No Imports";
		tvinsert.item.iImage=0;
		tvinsert.item.iSelectedImage=1;
		SendDlgItemMessage(hWnd,IDC_IMPORTS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		return;
	}
	// VA-Pointer to the offset
	delta = (INT)(pSection->VirtualAddress-pSection->PointerToRawData);
	
	// (Use that value to go to the first IMAGE_IMPORT_DESCRIPTOR structure )
	// Get the address of DLL pointers
	importDesc = (PIMAGE_IMPORT_DESCRIPTOR) (importsStartRVA - delta + (DWORD)pfile);
	if(importDesc==0)
	{
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
    
	   קריאת טבלת הפונקציות המשומשות ע"י הקובץ
	   זאת נעשה ע"י קריאת מילה שמשמשת כמצביע לשם הפוקציה
	   DLL אם המילה מכילה 0 אז עוברים ל
       עד שהוא מכיל 0 DLL וממשיכים לקרוא את טבלת המצביעים של ה
    */
	
	// Local variables
	HWND ExpTree;
	PIMAGE_EXPORT_DIRECTORY exportDir;	// Export struct
	PIMAGE_SECTION_HEADER header;		// Section struct
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