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


 File: SDK.h (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#ifndef _SDK_H_
#define _SDK_H_

#include <windows.h>
#include <stdio.h>
#include "../Functions.h"
#include <vector> // STL

using namespace std; // Use Standard Microsoft STL Templates

//////////////////////////////////////////////////////////////////////////
//								DEFINES									//
//////////////////////////////////////////////////////////////////////////

#define PI_BASE_MSG 200

// SDK PLUGIN MESSAGES
#define PI_GETASM					WM_USER+PI_BASE_MSG
#define PI_GETASMFROMINDEX			WM_USER+PI_BASE_MSG+1
#define PI_FLUSHDISASM				WM_USER+PI_BASE_MSG+2
#define PI_GETFUNCTIONEPFROMINDEX	WM_USER+PI_BASE_MSG+3
#define PI_GETENTRYPOINT			WM_USER+PI_BASE_MSG+4
#define PI_PRINTDBGTEXT				WM_USER+PI_BASE_MSG+5
#define PI_GETBYTEFROMADDRESS		WM_USER+PI_BASE_MSG+6
#define PI_RVATOFFSET				WM_USER+PI_BASE_MSG+7
#define PI_GETNUMOFSTRINGREF		WM_USER+PI_BASE_MSG+8
#define PI_GETSTRINGREFERENCE		WM_USER+PI_BASE_MSG+9
#define PI_SETCOMMENT				WM_USER+PI_BASE_MSG+10
#define PI_ADDCOMMENT				WM_USER+PI_BASE_MSG+11
#define PI_ADDFUNCTIONNAME			WM_USER+PI_BASE_MSG+12
#define PI_GETFUNCTIONNAME			WM_USER+PI_BASE_MSG+13
#define PI_GETCODESEGMENTSTARTEND	WM_USER+PI_BASE_MSG+14
#define PI_GET_NUMBER_OF_EXPORTS	WM_USER+PI_BASE_MSG+15
#define PI_GET_EXPORT_NAME			WM_USER+PI_BASE_MSG+16
#define PI_GET_EXPORT_DASM_INDEX	WM_USER+PI_BASE_MSG+17
#define PI_GET_EXPORT_ORDINAL		WM_USER+PI_BASE_MSG+18

#define WM_PLUGIN 2000

//////////////////////////////////////////////////////////////////////////
//								STRUCTS									//
//////////////////////////////////////////////////////////////////////////

typedef struct tagPluginInfo{

    HWND* Parent_hWnd;
    BYTE* FilePtr;
    DWORD FileSize;
    bool  DisasmReady;
    bool  LoadedPe;

}PLUGINFO;

//////////////////////////////////////////////////////////////////////////
//							TYPE DEFINES								//
//////////////////////////////////////////////////////////////////////////

typedef vector<HMODULE> DllArray;
// EXPORTS
typedef void  (*INSTALL)(PLUGINFO);
typedef char* (*PLG_INFO)();


//////////////////////////////////////////////////////////////////////////
//							PROTOTYPES									//
//////////////////////////////////////////////////////////////////////////

void EnumPlugins();
void LoadPlugins(HWND hWnd);
void FreePlugins();
void ExecutePlugin(DWORD dwIndex);
void CheckForPluginPress(WPARAM wParam);

#endif