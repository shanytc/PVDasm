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


 File: HexEditor.h (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#ifndef _HEX_EDITOR_H_
#define _HEX_EDITOR_H_

//////////////////////////////////////////////////////////////////////////
//							INCLUDES									//
//////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include "resource\resource.h"
#include <richedit.h>

// ================================================================
// ========================== DEFINES =============================
// ================================================================

// RadHexEdit Styles
#define STYLE_NOSPLITT		0x01  // No splitt button
#define STYLE_NOLINENUMBER	0x02  // No linenumber button
#define STYLE_NOHSCROLL		0x04  // No horizontal scrollbar
#define STYLE_NOVSCROLL		0x08  // No vertical scrollbar
#define STYLE_NOSIZEGRIP	0x10  // No size grip
#define STYLE_NOSTATE		0x20  // No state indicator
#define STYLE_NOADDRESS		0x40  // No adress field
#define STYLE_NOASCII		0x80  // No ascii field
#define STYLE_NOUPPERCASE	0x100 // Hex numbers is lowercase letters
#define STYLE_READONLY		0x200 // Text is locked

// Private messages
#define HEM_RAINIT				WM_USER+9999	// wParam=0, lParam=pointer to controls DIALOG struct
#define HEM_BASE				WM_USER+1000

// Messages
#define HEM_SETFONT				HEM_BASE+1	// wParam=nLineSpacing, lParam=lpRAFONT
#define HEM_GETFONT				HEM_BASE+2	// wParam=0, lParam=lpRAFONT
#define HEM_SETCOLOR			HEM_BASE+3	// wParam=0, lParam=lpRACOLOR
#define HEM_GETCOLOR			HEM_BASE+4	// wParam=0, lParam=lpRACOLOR
#define HEM_VCENTER				HEM_BASE+5	// wParam=0, lParam=0
#define HEM_REPAINT				HEM_BASE+6	// wParam=0, lParam=0
#define HEM_ANYBOOKMARKS		HEM_BASE+7	// wParam=0, lParam=0
#define HEM_TOGGLEBOOKMARK		HEM_BASE+8	// wParam=nLine, lParam=0
#define HEM_CLEARBOOKMARKS		HEM_BASE+9	// wParam=0, lParam=0
#define HEM_NEXTBOOKMARK		HEM_BASE+10	// wParam=0, lParam=0
#define HEM_PREVIOUSBOOKMARK	HEM_BASE+11	// wParam=0, lParam=0
#define HEM_SELBARWIDTH			HEM_BASE+12	// wParam=nWidth, lParam=0
#define HEM_LINENUMBERWIDTH		HEM_BASE+13	// wParam=nWidth, lParam=0
#define HEM_SETSPLIT			HEM_BASE+14	// wParam=nSplitt, lParam=0
#define HEM_GETSPLIT			HEM_BASE+15	// wParam=0, lParam=0

// Color Constants
#define DEFBCKCOLOR			0x00C0F0F0
#define DEFADRTXTCOLOR		0x00800000
#define DEFDTATXTCOLOR		0x00000000
#define DEFASCTXTCOLOR		0x00008000
#define DEFSELBCKCOLOR		0x00800000
#define DEFLFSELBCKCOLOR	0x00C0C0C0
#define DEFSELTXTCOLOR		0x00FFFFFF
#define DEFSELASCCOLOR		0x00C0C0C0
#define DEFSELBARCOLOR		0x00C0C0C0
#define DEFSELBARPEN		0x00808080
#define DEFLNRCOLOR		0x00800000

// ================================================================
// ========================== PROTOTYPES ==========================
// ================================================================

void LoadFileToRAHexEd			( HWND hWnd						);
void SaveFileFromRAHexEd		( HWND hWnd						);
void SaveFileAsFromRaHexEd		( HWND hWnd						);
void SaveFile					( HWND RadWin,char *FileName	);
void Hide_UnHide_RadHexAddress	( HWND hWnd						);
void Hide_UnHide_RadHexAscii	( HWND hWnd						);
void UpperLower_caseRadHex		( HWND hWnd						);

BOOL  CALLBACK HexEditorProc ( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam  );
DWORD CALLBACK StreamInProc  ( DWORD dwCookie, LPBYTE lpbBuff, LONG cb, LONG FAR *pcb );
DWORD CALLBACK StreamOutProc ( DWORD dwCookie, LPBYTE lpbBuff, LONG cb, LONG FAR *pcb );

#endif