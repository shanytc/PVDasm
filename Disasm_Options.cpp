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
 I have investigated P.E / Opcodes file(s)format as thoroughly as possible,
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


 File: Disasm_Options.cpp (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#include "Disasm.h"

void GetDisasmOptions(HWND hWnd, DISASM_OPTIONS *disopt)
{
	/* 
		Parameters: 
		-----------
	    hWnd - Widnow Handle
		*disopt - Disassembly Options struct

		Explanation:
		------------
		Procudure updates the DisasmOption struct
		With a visual defined options by the user.
		The procedure will check every Check Box button
		And will put 1/0 -> true/false in the right
		Member of the struct.
		Once filled we can use those option for our
		Visual disassembly 

		äôøåöãåøä úòãëï àú äîáðä áàåôöéåú
		ùðáçøå ò"é äîùúîù áçìåï äãéàìåâ
		ùì áçéøú àåôï äôéòðåç.
		ôøîèøéí:
		äôøåöãåøä î÷áìú àú äéãéú ìçìåï òìéä àðå òåáãéí
		åî÷áìú îöáéò ìîáðä àåúå àðå îòãëðéí
	*/

	// Show Remarks TRUE/FALSE
	if((SendDlgItemMessage(hWnd,IDC_SHOW_REMARKS,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
		disopt->ShowRemarks=1;
	else
		disopt->ShowRemarks=0;

	// Show Address TRUE/FALSE
	if((SendDlgItemMessage(hWnd,IDC_SHOW_ADDR,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
		disopt->ShowAddr=1;
	else
		disopt->ShowAddr=0;

	// Show Opcodes TRUE/FALSE
	if((SendDlgItemMessage(hWnd,IDC_SHOW_OPCODES,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
		disopt->ShowOpcodes=1;
	else
		disopt->ShowOpcodes=0;

	// Show UpperCase Assembly TRUE/FALSE
    if((SendDlgItemMessage(hWnd,IDC_UPPERCASE_DISASM,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
		disopt->UpperCased_Disasm=1;
	else
		disopt->UpperCased_Disasm=0;

	// Show & Analyze Sections TRUE/FALSE
	if((SendDlgItemMessage(hWnd,IDC_SHOW_SECTIONS,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
		disopt->ShowSections=1;
	else
		disopt->ShowSections=0;

	// Show Assembly TRUE/FALSE
	if((SendDlgItemMessage(hWnd,IDC_SHOW_ASSEMBLY,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
		disopt->ShowAssembly=1;
	else
		disopt->ShowAssembly=0;

    // Force Assembly before EP TRUE/FALSE
    if((SendDlgItemMessage(hWnd,IDC_FORCE_DISASM,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
        disopt->ForceDisasmBeforeEP=1;
    else
		disopt->ForceDisasmBeforeEP=0;

    // Start disassembly from EP only!
    if((SendDlgItemMessage(hWnd,IDC_FORCE_EP,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED){
        disopt->ForceDisasmBeforeEP=0;
        disopt->StartFromEP=1;
    }
    else
       disopt->StartFromEP=0;

    // FirstPass Disassembly
    if((SendDlgItemMessage(hWnd,IDC_FIRST_PASS,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
        disopt->FirstPass=1;
    else
		disopt->FirstPass=0;

	if((SendDlgItemMessage(hWnd,IDC_SIGNATURES,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED)
        disopt->Signatures=1;
    else
		disopt->Signatures=0;
}

void ShowDecoded(DISASSEMBLY disasm,DISASM_OPTIONS dops,HWND hDisasm,DWORD Index)
{
	/* 
	   This function will print info of the Disassembly
	   From the Disasm struct onto the
	   Dissassembly List Control.

       הצגת הקוד המפוענח על גבי חלון הרשימה
	*/

	LVITEM LvItem;
	char   Text[255];

	memset(&LvItem,0,sizeof(LvItem));
	LvItem.mask=LVIF_TEXT;
	LvItem.iItem=Index;
	LvItem.iSubItem=0;

	if(dops.ShowAddr==1){
		// put the info
		wsprintf(Text,"%08X",disasm.Address);
		LvItem.pszText=Text;
		SendMessage(hDisasm,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	}else{
		LvItem.pszText="";
		SendMessage(hDisasm,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	}

	if(dops.ShowOpcodes==1){
		LvItem.iSubItem=1;
		LvItem.pszText=disasm.Opcode;
		SendMessage(hDisasm,LVM_SETITEM,0,(LPARAM)&LvItem);
	}

	if(dops.ShowAssembly==1){
		LvItem.iSubItem=2;
		LvItem.pszText=disasm.Assembly;
		SendMessage(hDisasm,LVM_SETITEM,0,(LPARAM)&LvItem);
	}

	if(dops.ShowRemarks==1){
		LvItem.iSubItem=3;
		LvItem.pszText=disasm.Remarks;
		SendMessage(hDisasm,LVM_SETITEM,0,(LPARAM)&LvItem);
	}
}