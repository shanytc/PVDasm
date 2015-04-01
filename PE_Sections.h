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


 File: Pe_Sections.h (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/
#ifndef _PE_SECTIONS_H_
#define _PE_SECTIONS_H_

// ================================================================
// =========================  STRUCTS  ============================
// ================================================================
struct PeSection{	
	struct PeSection	*Next;
	DWORD				PhysicalAddress;
	DWORD				VirtualSize;
	DWORD				VirtualAddress;
	DWORD				SizeOfRawData;
	DWORD				PointerToRawData;
	DWORD				PointerToRelocations;
	DWORD				PointerToLinenumbers;
	DWORD				Characteristics;
	WORD				NumberOfRelocations;
	WORD				NumberOfLinenumbers;        
	char				Name[9];    	
};

// ================================================================
// =========================  DIALOGS  ============================
// ================================================================
BOOL CALLBACK PE_Sections			( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK Section_Header_Editor	( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK Sections_Table		( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

// ================================================================
// =========================  FUNCTIONS  ==========================
// ================================================================
int AddNodeToBeginning	( struct PeSection **headPtr		);
void SelListMake		( struct PeSection *MemPtr			);
void Set_PE_Info		( char *Data,HINSTANCE hInstance	);
void FreeList			( struct PeSection *Pointer			);
void Show_Sections		( struct PeSection *HeadPtr			);
void Save_Sections		( HWND hWnd							);
void Update_Pe			(									);
void DumpSection		( HWND hWnd							);
void DumpSectionBytes	( HANDLE hFile						);

#endif