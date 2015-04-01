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


 File: pvscript.h 
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

// Includes
#include <windows.h>
#include "..\x64.h"
#include "..\functions.h"
#include "..\resource\resource.h"
#include "..\Resize\AnchorResizing.h"
#include "..\Disasm.h"
#include <commctrl.h>
#include <iostream>
// STL Includes
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cctype>

// extern Variables
extern HWND	Main_hWnd;
extern MSG	msg;
extern bool FilesInMemory;
extern bool	DisassemblerReady;

// extern Functions
extern int	Open			( HWND hWnd	);
extern void CloseLoadedFile	( HWND hWnd	);
extern void ExitPVDasm		( HWND hWnd	);
extern BOOL DefineAddressesAsData(DWORD_PTR dwStart, DWORD_PTR dwEnd, BOOL mDisplayError);
BOOL DefineAddressesAsFunction(DWORD_PTR dwStart, DWORD_PTR dwEnd, char* FName, BOOL mDisplayError);

using namespace std;  // Use MicroSoft STL Templates

struct PVCommand{
	string m_commandName;
	vector<string> m_Parameters;
	int m_totalParameters;
};

typedef vector<string>::iterator sItr;
typedef string::iterator cItr;
typedef vector<PVCommand*>::iterator commandItr;

typedef struct FunctioInformation FUNCTION_INFORMATION;
typedef vector<FUNCTION_INFORMATION> FunctionInfo;
typedef FunctionInfo :: iterator FunctionInfoItr;
extern FunctionInfo fFunctionInfo;

typedef multimap<const int, int> MapTree;
extern MapTree DataAddersses;
typedef MapTree :: iterator TreeMapItr;

// Define Functions here
 enum StringValue {  notDefined,
				     cmd_close,
                     cmd_new,
                     cmd_define,
					 cmd_exit
                 };

class PVScript{
// PVScript Command Class
private:
	void ExecuteCommands();
	vector<string> *AllCommands;
	vector<string> *CommandParameters;
	vector<PVCommand*> ScriptCommands;
	map<string, StringValue> m_mapCommands;
	void DeleteCommands();

public:
	PVScript();
	~PVScript();
	void ParseCommands(string args);
	vector<string> *Parser(string command, string character);
	string GetCommand(int CommandPosition);
	DWORD_PTR GetTotalInstructions(){return AllCommands->size();}
};

// Window Procedures
BOOL CALLBACK ScriptEditorDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ScriptEditSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Function
void SendScriptMessage(string Message);
void DefineData(string addr_start, string addr_end, string command);
void DefineFunction(string addr_start, string addr_end, string fname, string command);
void DeleteData(string addr_start, string addr_end, string command);
void DeleteFunction(string addr_start, string addr_end, string command);
void ListData(string command);
void ListFunctions(string command);