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


 File: CDisasmData.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/
         
#include "disasm.h"
#include "Chip8.h"
#include "CDisasmData.h"
#include <vector>
#include <map>

using namespace std;  // Use Microsoft's STL Template Implemation

//////////////////////////////////////////////////////////////////////////
//						TYPE DEFINES									//
//////////////////////////////////////////////////////////////////////////

typedef vector<CDisasmData>			DisasmDataArray;
typedef multimap<const int, int>	XRef,MapTree;
typedef XRef::iterator				ItrXref;
typedef vector<CODE_BRANCH>			CodeBranch;
typedef pair<const int const, int>	XrefAdd,MapTreeAdd;

//////////////////////////////////////////////////////////////////////////
//							DEFINES										//
//////////////////////////////////////////////////////////////////////////

#define AddNew(key,data) XrefData.insert(XrefAdd(key,data));

//////////////////////////////////////////////////////////////////////////
//						EXTERNAL VARIABLES								//
//////////////////////////////////////////////////////////////////////////

extern DWORD_PTR		hFileSize;
extern char*			OrignalData;
extern DisasmDataArray	DisasmDataLines; 
extern XRef				XrefData;
extern DISASM_OPTIONS	disop; 
extern HWND				hWndTB,Main_hWnd;
extern bool				DisassemblerReady;
extern HANDLE			hDisasmThread;
extern CodeBranch		DisasmCodeFlow;

//////////////////////////////////////////////////////////////////////////
//						MAIN FUNCTION									//
//////////////////////////////////////////////////////////////////////////

void Chip8()
{    
	DISASSEMBLY Disasm;
	CODE_BRANCH CBranch;
	HWND hDisasm = GetDlgItem(Main_hWnd,IDC_DISASM);
	HMENU hMenu = GetMenu(Main_hWnd);
	DWORD_PTR Position=0;
	DWORD_PTR BytesToDecode = hFileSize;
	HWND DbgWin = GetDlgItem(Main_hWnd,IDC_LIST);
	DWORD_PTR BrachAddress=0;
	DWORD_PTR Index=0,ListIndex=0,percent=0,step=-1;
	WORD Opcode=0;
	char* Data=OrignalData;

	SendDlgItemMessage(Main_hWnd,IDC_DISASM_PROGRESS,PBM_SETRANGE32,(WPARAM)0,(LPARAM)BytesToDecode);
	ShowWindow(GetDlgItem(Main_hWnd,IDC_DISASM_PROGRESS),SW_SHOW);
	EnableMenuItem(hMenu,IDC_START_DISASM,MF_GRAYED);
	// Disable ToolBar Call/Jump Tracing buttons
	SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,  (LPARAM) FALSE );
	SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,  (LPARAM) FALSE );
	SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_EXEC_JUMP, (LPARAM) FALSE );
	SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_EXEC_CALL, (LPARAM) FALSE );

	OutDebug(Main_hWnd,"+----------------------------------------------------+");
	OutDebug(Main_hWnd,"|           Disassembly Analysis          |");
	OutDebug(Main_hWnd,"+----------------------------------------------------+");
	Sleep(55);
	SelectLastItem(DbgWin); // Selects the Last Item
	    
	OutDebug(Main_hWnd,"Fetching Entry Point: 0x0200");
	Sleep(55);
	SelectLastItem(DbgWin); // Selects the Last Item

	// code starts at address 0x200
	Disasm.Address=0x00000200;
    
	for(;Index<BytesToDecode;Index+=2,ListIndex++){ // Decode all Bytes
		// Size of each Opcode is 2 bytes,
		// Build Opcode:
		percent=signed int(((float)Index/(float)BytesToDecode)*100.0);

		Opcode = (WORD)(Data[Index]<<8);
		Opcode |= ((WORD)(Data[Index+1])&0x00FF);

		BrachAddress=0;
		FlushDecoded(&Disasm); //Flush Struct content
		Disasm8Chip(Opcode,&Disasm,&BrachAddress);  // Decode the Opcode

		if(Disasm.CodeFlow.Call == TRUE || Disasm.CodeFlow.Jump == TRUE){
			if( ( BrachAddress >= 0x200 ) && ( BrachAddress <= BytesToDecode ) ){
				AddNew((DWORD)BrachAddress,(DWORD)ListIndex)

				// Update Information
				CBranch.Current_Index = ListIndex;
				CBranch.Branch_Destination=BrachAddress;
				CBranch.BranchFlow.Call = Disasm.CodeFlow.Call;
				CBranch.BranchFlow.Jump = Disasm.CodeFlow.Jump;

				// Save Data
				DisasmCodeFlow.insert(DisasmCodeFlow.end(),CBranch);
			}
		}
        
		wsprintf(Disasm.Opcode,"%04X",Opcode);        
		Disasm.OpcodeSize=2;
		AddNewDecode(Disasm,(DWORD)ListIndex); // Add Struct to the List
		Disasm.Address+=Disasm.OpcodeSize;  
		Position+=Disasm.OpcodeSize;
		if(percent>step){
			step=percent;
			// Advance the ProgressBar
			PostMessage(GetDlgItem(Main_hWnd,IDC_DISASM_PROGRESS),PBM_SETPOS,(WPARAM)Position,0);
			PostMessage(GetDlgItem(Main_hWnd,IDC_DISASM_PROGRESS),PBM_STEPIT,0,0);
			PostMessage(Main_hWnd,WM_USER+MY_MSG,0,(LPARAM)Disasm.Address);
		}
	}

	DisassemblerReady=TRUE;
	ShowWindow(GetDlgItem(Main_hWnd,IDC_DISASM_PROGRESS),SW_HIDE);
	SendDlgItemMessage(Main_hWnd,IDC_DISASM_PROGRESS,PBM_SETPOS,0,0);
	ListView_SetItemCountEx(hDisasm,ListIndex,NULL);
	// Disable Stop Disassembly menu item
	EnableMenuItem ( hMenu, IDC_STOP_DISASM,  MF_GRAYED ); 
	EnableMenuItem ( hMenu, IDC_PAUSE_DISASM, MF_GRAYED );
	EnableMenuItem ( hMenu, IDC_START_DISASM, MF_ENABLED);
	SendMessage    ( hWndTB, TB_ENABLEBUTTON, ID_SEARCH, (LPARAM) TRUE );
	EnableMenuItem ( hMenu, IDM_FIND, MF_ENABLED );

	// Show Disassembly Summery:
	// ========================
	SetDlgItemText ( Main_hWnd, IDC_MESSAGE1,"Disassembly Analysis Complete.");
	SetDlgItemText ( Main_hWnd, IDC_MESSAGE2,"");
	SetDlgItemText ( Main_hWnd, IDC_MESSAGE3,"");

	OutDebug(Main_hWnd,"");
	OutDebug(Main_hWnd,"Disassembly Analysis Complete.");
	OutDebug(Main_hWnd,"------------------------------------------------------");
	Sleep(55);
	SelectLastItem(DbgWin); // Selects the Last Item

	CloseHandle(hDisasmThread);
	ExitThread(0);
}

//////////////////////////////////////////////////////////////////////////
//				ADD NEW DECODED LINE TO LIST							//
//////////////////////////////////////////////////////////////////////////

void AddNewDecode(DISASSEMBLY Disasm,DWORD Index){
	CDisasmData DisasmData("","","","",0,0,"");
	char Text[128];

	DisasmDataLines.insert(DisasmDataLines.end(),DisasmData);
	assert(DisasmDataLines.size() == Index+1 && DisasmDataLines.capacity() >= Index+1);

	if(disop.ShowAddr==TRUE){
		wsprintf(Text,"%08X",Disasm.Address);
		DisasmDataLines[Index].SetAddress(Text);
	}else{
		DisasmDataLines[Index].SetAddress("");
	}

	if(disop.ShowOpcodes==TRUE){
		DisasmDataLines[Index].SetCode(Disasm.Opcode);
	}
	else{
		DisasmDataLines[Index].SetCode("");
	}


	if(disop.ShowAssembly==TRUE){
		DisasmDataLines[Index].SetMnemonic(Disasm.Assembly);
	}
	else{
		DisasmDataLines[Index].SetMnemonic("");
	}


	if(disop.ShowRemarks==TRUE){
		DisasmDataLines[Index].SetComments(Disasm.Remarks);
	}
	else{
		DisasmDataLines[Index].SetComments("");
	}

	DisasmDataLines[Index].SetSize((DWORD)Disasm.OpcodeSize);
	DisasmDataLines[Index].SetPrefixSize((DWORD)Disasm.PrefixSize);
}

//////////////////////////////////////////////////////////////////////////
//						DISASSEMBLER ENGINE								//
//////////////////////////////////////////////////////////////////////////

void Disasm8Chip(WORD wOpcode,DISASSEMBLY* Disasm,DWORD_PTR* Address)
{    
	CHAR8 Opcode = (wOpcode & 0xF000)>>12; // Get Opcode Main Number (X000)

	switch(Opcode){
		case 0x00:{
			switch(wOpcode&0x00FF){
				case 0xE0: lstrcpy(Disasm->Assembly,"cls");		break;
				case 0xEE: lstrcpy(Disasm->Assembly,"rts");		break;
				case 0xFB: lstrcpy(Disasm->Assembly,"scright");	break;
				case 0xFC: lstrcpy(Disasm->Assembly,"scleft");	break;
				case 0xFE: lstrcpy(Disasm->Assembly,"low");		break;
				case 0xFF: lstrcpy(Disasm->Assembly,"high");	break;
				default: wsprintf(Disasm->Assembly,"scdown %d",wOpcode&0x000F); break;
			}
		}
		break;

		case 0x01:{
			wsprintf(Disasm->Assembly,"jmp %03X",wOpcode&0x0FFF); 
			Disasm->CodeFlow.Jump=TRUE;  
			*Address=wOpcode&0x0FFF;
		}
		break;

		case 0x02:{
			wsprintf(Disasm->Assembly,"jsr %03X",wOpcode&0x0FFF); 
			Disasm->CodeFlow.Call=TRUE;  
			*Address=wOpcode&0x0FFF;
		}
		break;

		case 0x03:{
			wsprintf(Disasm->Assembly,"skeq v%d,%X",(wOpcode&0x0F00)>>8,wOpcode&0x00FF);
			Disasm->CodeFlow.Jump=TRUE;
			*Address=Disasm->Address+4;
		}
		break;

		case 0x04:{
			wsprintf(Disasm->Assembly,"skne v%d,%X",(wOpcode&0x0F00)>>8,wOpcode&0x00FF); 
			Disasm->CodeFlow.Jump=TRUE;
			*Address=Disasm->Address+4;
		}
		break;

		case 0x05:{
			wsprintf(Disasm->Assembly,"skeq v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);
			Disasm->CodeFlow.Jump=TRUE;
			*Address=Disasm->Address+4;
		}
		break;

		case 0x06: wsprintf(Disasm->Assembly,"mov v%d,%X",(wOpcode&0x0F00)>>8,wOpcode&0x00FF);	break;
		case 0x07: wsprintf(Disasm->Assembly,"add v%d,%X",(wOpcode&0x0F00)>>8,wOpcode&0x00FF);	break;
		case 0x08:{
			switch(wOpcode&0x000F){
				case 0x00: wsprintf(Disasm->Assembly,"mov v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
				case 0x01: wsprintf(Disasm->Assembly,"or v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
				case 0x02: wsprintf(Disasm->Assembly,"and v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
				case 0x03: wsprintf(Disasm->Assembly,"xor v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
				case 0x04: wsprintf(Disasm->Assembly,"add v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
				case 0x05: wsprintf(Disasm->Assembly,"sub v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
				case 0x06: wsprintf(Disasm->Assembly,"shr v%d",(wOpcode&0x0F00)>>8);							break;
				case 0x07: wsprintf(Disasm->Assembly,"rsb v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
				case 0x0E: wsprintf(Disasm->Assembly,"shl v%d",(wOpcode&0x0F00)>>8);							break;
				default:	wsprintf(Disasm->Assembly,"???");													break;
			}
		}
		break;

		case 0x09: wsprintf(Disasm->Assembly,"skne v%d,v%d",(wOpcode&0x0F00)>>8,(wOpcode&0x00F0)>>4);	break;
		case 0x0A: wsprintf(Disasm->Assembly,"mvi %03X",wOpcode&0x0FFF);								break;
		case 0x0B:{
			wsprintf(Disasm->Assembly,"jmi %03X",wOpcode&0x0FFF);
			Disasm->CodeFlow.Jump=TRUE;
			*Address=wOpcode&0x0FFF;
		}
		break;

		case 0x0C: wsprintf(Disasm->Assembly,"rand v%d,%03X",(wOpcode&0x0F00)>>8,wOpcode&0x0FFF);	break;
		case 0x0D:{
			switch(wOpcode&0x000F){
				case 0x00: wsprintf(Disasm->Assembly, "xsprite %d,%d",(wOpcode&0x0F00)>>8, (wOpcode&0x00F0)>>4);					break;
				default: wsprintf(Disasm->Assembly, "sprite %d,%d,%d",(wOpcode&0x0F00)>>8, (wOpcode&0x00F0)>>4,wOpcode&0x000F);	break;
			}
		}
		break;

		case 0x0E:{
			switch(wOpcode&0x00FF){
				case 0x9E: wsprintf(Disasm->Assembly,"skpr %d",(wOpcode&0x0F00)>>8);	break;
				case 0xA1: wsprintf(Disasm->Assembly,"skup %d",(wOpcode&0x0F00)>>8);	break;
				default:   wsprintf(Disasm->Assembly,"???");							break;
			}
		}
		break;

		case 0x0F:{
			switch(wOpcode&0x00FF){
				case 0x07: wsprintf(Disasm->Assembly,"gdelay v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x0A: wsprintf(Disasm->Assembly,"key v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x15: wsprintf(Disasm->Assembly,"sdelay v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x18: wsprintf(Disasm->Assembly,"ssound v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x1E: wsprintf(Disasm->Assembly,"adi v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x29: wsprintf(Disasm->Assembly,"font v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x30: wsprintf(Disasm->Assembly,"xfont v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x33: wsprintf(Disasm->Assembly,"bcd v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x55: wsprintf(Disasm->Assembly,"str v0-v%d",(wOpcode&0x0F00)>>8);	break;
				case 0x65: wsprintf(Disasm->Assembly,"ldr v0-v%d",(wOpcode&0x0F00)>>8);	break;
				default:	wsprintf(Disasm->Assembly,"???");							break;
			}
		} 
		break;
        
		default: wsprintf(Disasm->Assembly,"???");	break;
	}
}