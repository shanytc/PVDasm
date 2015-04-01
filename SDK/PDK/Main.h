#ifndef _DLL_H_
#define _DLL_H_

//////////////////////////////////////////////////////////////////////////
//                         INCLUDES                                     //
//////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <STDIO.H>
#include <COMMCTRL.H>
#include "Res/resource.h"

// Export Type
#define DLL_EXPORT __declspec(dllexport)


// This is the Plugin Description which will be 
// Shown inside PVDasm
#define PLUGIN_NAME "Plugin Test" // No More than 128 characters!!

//////////////////////////////////////////////////////////////////////////
//                          MESSAGES                                    //
//////////////////////////////////////////////////////////////////////////

// PLUGIN MESSAGES
#define PI_BASE_MSG 200
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
#define PI_SETDISASM				WM_USER+PI_BASE_MSG+10
#define PI_ADDCOMMENT				WM_USER+PI_BASE_MSG+11
#define PI_ADDFUNCTIONNAME			WM_USER+PI_BASE_MSG+12
#define PI_GETFUNCTIONNAME			WM_USER+PI_BASE_MSG+13
#define PI_GETCODESEGMENTSTARTEND	WM_USER+PI_BASE_MSG+14
#define PI_GET_NUMBER_OF_EXPORTS	WM_USER+PI_BASE_MSG+15
#define PI_GET_EXPORT_NAME			WM_USER+PI_BASE_MSG+16
#define PI_GET_EXPORT_DASM_INDEX	WM_USER+PI_BASE_MSG+17
#define PI_GET_EXPORT_ORDINAL		WM_USER+PI_BASE_MSG+18

// PVDasm Control IDs
#define ID_DISASM        1072
#define ID_DEBUG_WINDOW  1060

//////////////////////////////////////////////////////////////////////////
//                          STRUCTS                                     //
//////////////////////////////////////////////////////////////////////////

typedef struct FunctioInformation{
	
	// Define a Function Name, start and end.
	DWORD FunctionStart,FunctionEnd;
	char FunctionName[50];
	
} FUNCTION_INFORMATION;

typedef struct tagPluginInfo{

    HWND* Parent_hWnd;
    BYTE* FilePtr;
    DWORD FileSize;
    bool  DisasmReady;
    bool  LoadedPe;

}PLUGINFO; // Plugin Struct

typedef struct Code_Flow{

    bool Jump;			// Instruction is a Jcc / jmp
    bool Call;			// Instruction is a Call
    bool BranchSize;	// Short / Near
    bool Ret;			// Instruction is Ret

} CODE_FLOW;

typedef struct Decoded{
	
	// Define Decoded instruction struct

    DWORD     Address;       // Current address of decoded instruction
    CODE_FLOW CodeFlow;      // InstructionS: Jump or Call 
    BYTE      OpcodeSize;    // Opcode Size
	BYTE      PrefixSize;    // Size of all prefixes used
    char      Assembly[128]; // Menemonics
    char      Remarks[256];  // Menemonic addons
    char      Opcode[25];    // Opcode Byte forms

} DISASSEMBLY;

//////////////////////////////////////////////////////////////////////////
//                        FUNCTION EXPORTS                              //
//////////////////////////////////////////////////////////////////////////

extern "C" { 
    
    DLL_EXPORT void  InstallDll(PLUGINFO);  
    DLL_EXPORT char* PluginInfo();
};

//////////////////////////////////////////////////////////////////////////
//                           PROTOTYPES                                 //
//////////////////////////////////////////////////////////////////////////

BOOL CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif