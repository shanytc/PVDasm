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

 File: pvscript.cpp (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#include "pvscript.h"

WNDPROC OldScriptEditWndProc;
HWND ScriptWindow;

PVScript::PVScript(){
	AllCommands = new vector<string>;
	AllCommands=NULL;

	CommandParameters = new vector<string>;
	CommandParameters = NULL;

	m_mapCommands["close"] = cmd_close;
	m_mapCommands["new"] = cmd_new;
	m_mapCommands["define"] = cmd_define;
	m_mapCommands["exit"] = cmd_exit;
}

PVScript::~PVScript(){
	DeleteCommands();
	ScriptCommands.erase(ScriptCommands.begin(),ScriptCommands.end());
	ScriptCommands.clear();
	vector<PVCommand*>(ScriptCommands).swap(ScriptCommands);
}

void PVScript::DeleteCommands(){
	if(AllCommands){
		delete AllCommands;
		AllCommands = NULL;
	}
	
	if(CommandParameters){
		delete CommandParameters;
		CommandParameters=NULL;
	}
}

void PVScript::ParseCommands(string args){
	AllCommands = Parser(args,";");
	DWORD_PTR total_instructions = GetTotalInstructions();
	PVCommand *cmd;
	string cmd_name;
	
	for(int i=0;i<total_instructions;i++)
	{
		cmd = new PVCommand;
		cmd->m_totalParameters = 0;
		CommandParameters = Parser(GetCommand(i)," ");
		
		sItr iter = CommandParameters->begin();
		
		// check if the command is naked
		if(iter!=CommandParameters->end()){
			cmd->m_commandName = *iter;
			// skip first parameter (command name)
			iter++;
			for(;iter!=CommandParameters->end();iter++){
				cmd_name = (*iter);
				cmd->m_Parameters.push_back(cmd_name);
				cmd->m_totalParameters++;
			}
		}

		ScriptCommands.push_back(cmd);
	}

	DeleteCommands();
	ExecuteCommands();
}

void PVScript::ExecuteCommands(){
	commandItr cItr = ScriptCommands.begin();
	string command;

	for(;cItr!=ScriptCommands.end();*cItr++){
		sItr params = (*cItr)->m_Parameters.begin();
		command = (*cItr)->m_commandName;
		switch(m_mapCommands[command]) {
			case cmd_close:
				if(FilesInMemory==true && DisassemblerReady==true){
					SendScriptMessage("\r\n"+command+";;");
					SendScriptMessage("\r\n// Result: Closing working file // ");
					CloseLoadedFile(Main_hWnd);
				}{
					SendScriptMessage("\r\n"+command+";;");
					SendScriptMessage("\r\n// Error: Can't close working file. // ");
				}
			break;

			case cmd_new:{
				SendScriptMessage("\r\n"+command+";;");
				SendScriptMessage("\r\n// Result: Opening new file for disassembly // ");
				Open(Main_hWnd);
				/*
				if(params!=(*cItr)->m_Parameters.end()){	
					string file = *params;
				}
				*/
			}	
			break;

			case cmd_exit:{
				//EndDialog(ScriptWindow,0);
				ExitPVDasm(Main_hWnd);
			}
			break;

			// DEFINE Command/Sub Commands
			//
			// Command List
			// define -d XXXXXXXX XXXXXXXX;				- defines a data range.
			// define -f XXXXXXXX XXXXXXXX [NAME];		- defined a new function (name is optional).
			// define -x [-d]/[-f] XXXXXXXX XXXXXXXX;	- removes a data or function range.
			// define -l [-d]/[-f];						- list defined functions or data.
			case cmd_define:{
				if(FilesInMemory && DisassemblerReady){
					if(params!=(*cItr)->m_Parameters.end()){
						string sub_command = *params;
						
						if(sub_command=="-d") // Define Data
						{
							params++;
							if(params==(*cItr)->m_Parameters.end()){
								SendScriptMessage("\r\n"+command+";;");
								SendScriptMessage("\r\n// Error: Flag '-d' expects 2 argument(s). // ");
								break;
							}
							string address_1 = *params;
							params++;
							string address_2 = *params;
							DefineData(address_1,address_2,command);
							break;
						}

						if(sub_command=="-f") // Define Function
						{
							params++;
							if(params==(*cItr)->m_Parameters.end())
							{
								SendScriptMessage("\r\n"+command+";;");
								SendScriptMessage("\r\n// Error: Flag '-f' expects 2 argument(s). // ");
								break;
							}
							string address_1 = *params;
							params++;
							string address_2 = *params;
							params++;
							string fname="";
							if(params!=(*cItr)->m_Parameters.end()){
								fname = *params;
							}
							DefineFunction(address_1,address_2,fname,command);
							break;
						}

						if(sub_command=="-l") // Display/List Defined Addresses
						{
							params++;
							if(params==(*cItr)->m_Parameters.end())
							{
								SendScriptMessage("\r\n"+command+";;");
								SendScriptMessage("\r\n// Error: Flag '-l' expects 2 argument(s). // ");
								break;
							}
							string type = *params; // Type to remove, -d for data / -f for function
							params++;
							if(type=="-d"){
								ListData(command);
								break;
							}
							if(type=="-f"){
								ListFunctions(command);
								break;
							}
							break;
						}

						if(sub_command=="-x") // Remove Defined Addresses
						{
							params++;
							if(params==(*cItr)->m_Parameters.end())
							{
								SendScriptMessage("\r\n"+command+";;");
								SendScriptMessage("\r\n// Error: Flag '-x' expects 3 argument(s). // ");
								break;
							}

							string type = *params; // Type to remove, -d for data / -f for function
							params++;
							string address_1 = *params;
							params++;
							string address_2 = *params;
							params++;
							if(type=="-d"){
								DeleteData(address_1,address_2,command);
								break;
							}
							if(type=="-f"){
								DeleteFunction(address_1,address_2,command);
								break;
							}
							break;
						}

						SendScriptMessage("\r\n"+command+";;");
						SendScriptMessage("\r\n// Error: Unrecognized command parameter. // ");
					}
				}
				else{
					SendScriptMessage("\r\n"+command+";;");
					SendScriptMessage("\r\n// Error: Disassembly not ready. // ");
				}
			}
		}
	}
}

vector<string> *PVScript::Parser(string args, string character){
	DWORD_PTR size;

	string command;
	vector<string> *Commands = new vector<string>;
	
	// Explode Commands

	while(TRUE){
		size = args.find(character);
		if(size==-1){
			if(!(args=="")){
				Commands->push_back(args);
			}
			break;
		}

		if(size!=NULL){
			command = args.substr(0,size);
			Commands->push_back(command);
			&args.erase(0,size+1);
		}else{
			&args.erase(0,size+1);
		}
	}

	return Commands;
}

string PVScript::GetCommand(int CommandPosition){
	
	vector<string>::iterator commandLooper;
	commandLooper = AllCommands->begin();
	return commandLooper[CommandPosition];
}

void SendScriptMessage(string Message)
{
	int length = GetWindowTextLength(GetDlgItem(ScriptWindow,IDC_SCRIPT_MESSAGE));
	char *commands = new char[length]+1;
	GetDlgItemText(ScriptWindow,IDC_SCRIPT_MESSAGE,commands,length);
	string cmd = commands+Message;
	SetDlgItemText(ScriptWindow,IDC_SCRIPT_MESSAGE,cmd.c_str());
}

void DefineData(string addr_start, string addr_end, string command)
{
	SendScriptMessage("\r\n"+command+";;");

	if(addr_start.size()!=8 || addr_end.size()!=8){
		SendScriptMessage("\r\n// Error: start address or end address has invalid length // ");
		return;
	}

	DWORD dwStart = StringToDword((char*)addr_start.c_str());
	DWORD dwEnd = StringToDword((char*)addr_end.c_str());

	if(dwEnd<=dwStart){
		SendScriptMessage("\r\n// Error: end Address can't be <= to start Address // ");
	}

	if(DefineAddressesAsData(dwStart,dwEnd, FALSE)) // define the area
	{
		SendScriptMessage("\r\n// Result: Addresses "+ addr_start + "-" + addr_end + " defined as data. // ");
	}else{
		SendScriptMessage("\r\n// Result: Addresses "+ addr_start + "-" + addr_end + " already defined as data. // ");
	}
}

void ListData(string command){
	DWORD_PTR dwStart;
	DWORD_PTR dwEnd;
	DWORD_PTR len;
	string textTemlate = "\r\n// %d. %08X - %08X // ";
	SendScriptMessage("\r\n"+command+";;");
	TreeMapItr itr=DataAddersses.begin();
	for(int i=1;itr!=DataAddersses.end();itr++,i++)
	{
		dwStart = (*itr).first;
		dwEnd = (*itr).second;
		len = sizeof(DWORD_PTR)+(sizeof(DWORD_PTR)*2)+textTemlate.length()+1;
		char* output = new char[len];
		RtlZeroMemory(output,len);
		sprintf_s(output,len,textTemlate.c_str(),i,dwStart,dwEnd);
		SendScriptMessage(output);
		delete output;
	}
}

void ListFunctions(string command){
	DWORD_PTR dwStart;
	DWORD_PTR dwEnd;
	string FName;
	DWORD_PTR len;
	string textTemlate = "\r\n// %d. '%s' %08X - %08X // ";
	SendScriptMessage("\r\n"+command+";;");
	FunctionInfoItr itr=fFunctionInfo.begin();
	for(int i=1;itr!=fFunctionInfo.end();itr++,i++)
	{
		dwStart = (*itr).FunctionStart;
		dwEnd = (*itr).FunctionEnd;
		FName = (*itr).FunctionName;
		len = sizeof(DWORD_PTR)+(sizeof(DWORD_PTR)*2)+FName.length()+textTemlate.length()+1;
		char* output = new char[len];
		RtlZeroMemory(output,len);
		sprintf_s(output,len,textTemlate.c_str(),i,FName.c_str(),dwStart,dwEnd);
		SendScriptMessage(output);
		delete output;
	}
}

void DeleteData(string addr_start, string addr_end, string command)
{
	SendScriptMessage("\r\n"+command+";;");
	DWORD dwStart = StringToDword((char*)addr_start.c_str());
	TreeMapItr itr = DataAddersses.find(dwStart);
	if(itr!=DataAddersses.end()){
		DataAddersses.erase(dwStart);
		SendScriptMessage("\r\n// Result: Data at Addresses "+ addr_start + "-" + addr_end + " undefined. // ");
	}else{
		SendScriptMessage("\r\n// Error: Data at Addresses "+ addr_start + "-" + addr_end + " is not defined. // ");
	}
}

void DeleteFunction(string addr_start, string addr_end, string command)
{
	BOOL Found=FALSE;
	FunctionInfoItr itr=fFunctionInfo.begin();

	SendScriptMessage("\r\n"+command+";;");
	
	DWORD dwStart = StringToDword((char*)addr_start.c_str());
	for(;itr!=fFunctionInfo.end();itr++)
	{
		if( (*itr).FunctionStart==dwStart )
		{
			Found=TRUE;
			break;
		}
	}
	
	if(Found==TRUE){
		fFunctionInfo.erase(itr);
		SendScriptMessage("\r\n// Result: Function at addresses "+ addr_start + "-" + addr_end + " undefined. // ");
		return;
	}
	
	SendScriptMessage("\r\n// Error: Can't undefined function at "+ addr_start + "-" + addr_end + " // ");
}

void DefineFunction(string addr_start, string addr_end, string fname, string command)
{
	SendScriptMessage("\r\n"+command+";;");

	if(addr_start.size()!=8 || addr_end.size()!=8){
		SendScriptMessage("\r\n// Error: start address or end address has invalid length // ");
		return;
	}

	DWORD dwStart = StringToDword((char*)addr_start.c_str());
	DWORD dwEnd = StringToDword((char*)addr_end.c_str());

	if(dwEnd<=dwStart){
		SendScriptMessage("\r\n// Error: End address can't be <= to start Address // ");
	}

	if(DefineAddressesAsFunction(dwStart,dwEnd, (char*)fname.c_str(), FALSE)) // define the area
	{
		SendScriptMessage("\r\n// Result: '"+fname+"' "+ addr_start + "-" + addr_end + " defined as function. // ");
	}else{
		SendScriptMessage("\r\n// Result: '"+fname+"' "+ addr_start + "-" + addr_end + " already defined as function. // ");
	}
}

//
// Dialog Functions
//
BOOL CALLBACK ScriptEditorDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: 
		{	
		    InitCommonControls();
			// Make the window resizble with a specific properties
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_SCRIPT_MESSAGE), GWL_USERDATA,ANCHOR_RIGHT  | ANCHOR_BOTTOM);
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_SCRIPT_EDIT), GWL_USERDATA,ANCHOR_BOTTOM | ANCHOR_RIGHT | ANCHOR_NOT_TOP);

			// Subclass the edit window
			OldScriptEditWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_SCRIPT_EDIT),GWL_WNDPROC,(LONG_PTR)ScriptEditSubClass);
			ScriptWindow = hWnd;
			SetDlgItemText(hWnd,IDC_SCRIPT_MESSAGE,"// PVScript Engine ");
			SetDlgItemText(hWnd,IDC_SCRIPT_EDIT," ");
			ShowWindow(hWnd,0);
			UpdateWindow(hWnd);
		}
		break;

		case WM_PAINT:
		{
		   return FALSE;
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

        case WM_CLOSE: // We colsing the Dialog
        {
			InitializeResizeControls(Main_hWnd);
			EndDialog(hWnd,0); 
        }
	    break;
	}
	return 0;
}

LRESULT CALLBACK ScriptEditSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
		// All keys are acceptble
	    case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS;
		}
		break;

        case WM_KEYDOWN:
        {
			switch(wParam)
			{
				// See if we can trace the call/jmp
				case VK_RETURN:
				{   
					if(GetKeyState(VK_CONTROL) & 0x8000000){ // ctrl is down
						PeekMessage(&msg, 0, WM_CHAR, WM_CHAR, PM_REMOVE);
						// Parse and execute script
						int length = GetWindowTextLength(GetDlgItem(ScriptWindow,IDC_SCRIPT_EDIT));
						char *commands = new char[length]+1;
						GetDlgItemText(ScriptWindow,IDC_SCRIPT_EDIT,commands,length);
						string cmd = commands;
						transform(cmd.begin(),cmd.end(),cmd.begin(),tolower);
						PVScript ScriptEngine;// PVScript script engine
						while(1){
							DWORD_PTR pos = cmd.find("\r\n");
							if(pos==-1){break;}
							cmd.replace(pos,2,"");
						}
						ScriptEngine.ParseCommands(cmd);
					}
				}
				break;
			}
        }
        break;
    }

    return CallWindowProc(OldScriptEditWndProc, hWnd, uMsg, wParam, lParam);
}
