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

 File: Wizard.cpp
   This program was written by Shany Golan, Student at :
	Ruppin, department of computer science and engineering University.
*/

#include "Wizard.h"
#include "Functions.h"
#include "Disasm.h"

//////////////////////////////////////////////////////////////////////////
//						TYPE DEFINES									//
//////////////////////////////////////////////////////////////////////////
typedef vector<CODE_BRANCH>				CodeBranch;
typedef vector<FUNCTION_INFORMATION>	FunctionInfo;

//////////////////////////////////////////////////////////////////////////
//						EXTERNAL VARIABLES								//
//////////////////////////////////////////////////////////////////////////

extern DisasmImportsArray	StrRefLines;
extern DisasmDataArray		DisasmDataLines;
extern XRef					XrefData; 
extern IMAGE_NT_HEADERS*	nt_hdr;	
extern HANDLE				hDisasmThread;
extern DWORD				DisasmThreadId;
extern DisasmImportsArray	ImportsLines;
extern CodeBranch			DisasmCodeFlow;
extern FunctionInfo			fFunctionInfo;

//////////////////////////////////////////////////////////////////////////
//							DEFINES										//
//////////////////////////////////////////////////////////////////////////

#define AddNewDataEntry(key,data) DataSection.insert(DataMapTreeAdd(key,data));
#define AddString(String) WizardCodeList.insert(WizardCodeList.end(),String);

#define USE_MMX

#ifdef USE_MMX
	#define StringLen(str) xlstrlen(str) // new lstrlen with mmx function
#else
    #define StringLen(str) lstrlen(str) // old c library strlen
#endif

//////////////////////////////////////////////////////////////////////////
//							GLOBAL VARIABLES							//
//////////////////////////////////////////////////////////////////////////

DataMapTree			DataSection;
CodeSectionArray	CodeSections;
WIZARD_OPTIONS		WizardOptions;
WizardList			WizardCodeList;

HWND  PrevhWnd;
DWORD OEP=0;

char prjFileName[MAX_PATH]="";
char MasmPath[MAX_PATH]="";
char WizardBuffer[256]="";

int VarCount=1;
bool FirstParam=TRUE;

//////////////////////////////////////////////////////////////////////////
//							FUNCTIONS									//
//////////////////////////////////////////////////////////////////////////

BOOL CALLBACK WizardDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG:{	
		    SetWindowText(GetDlgItem(hWnd,IDC_MASM_PRJ_PATH_EDIT),prjFileName);	
            SetWindowText(GetDlgItem(hWnd,IDC_MASM_PATH_EDIT),MasmPath);
		}
		break;

		case WM_PAINT:{
		   return FALSE;
		}
		break;
		
        case WM_COMMAND:
        {
            switch(LOWORD(wParam)){

				case IDC_WIZARD_PATH_CANCEL:{
				   SendMessage(hWnd,WM_CLOSE,0,0);
				}
				break;
				
				case IDC_PRJ_PATH_SELECT:{
                    OPENFILENAME ofn;
                    // Intialize struct
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of OPENFILENAME Struct
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = "Asm Files (*.asm)\0*.asm\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = prjFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt = "asm";

                    if(GetSaveFileName(&ofn)!=NULL){
                        SetWindowText(GetDlgItem(hWnd,IDC_MASM_PRJ_PATH_EDIT),prjFileName);
                    }
                    else return FALSE;
				}
				break;

                case IDC_MASM_PATH_SELECT:{
                    ITEMIDLIST* iIDl;
                    MasmPath[0]=NULL;
                    BROWSEINFO lpbi = {NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL};
                    iIDl = SHBrowseForFolder(&lpbi);
                    SHGetPathFromIDList(iIDl, MasmPath);
                    if(MasmPath[0]!=NULL){
                        SetWindowText(GetDlgItem(hWnd,IDC_MASM_PATH_EDIT),MasmPath);
                    }
                }
                break;

                case IDC_WIZARD_PATH_NEXT:{
                    EndDialog(hWnd,0);
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD_DATA), hWnd, (DLGPROC)WizardDataDlgProc);
                }
                break;
            }
        }
        break;

        case WM_CLOSE:{ // We colsing the Dialog
          EndDialog(hWnd,0); 
        }
	    break;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
//					Define .Data String variables						//
//////////////////////////////////////////////////////////////////////////

BOOL CALLBACK WizardDataDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) // what are we doing ?
	{ 	 
		case WM_INITDIALOG:{	
            LV_COLUMN	LvCol;
            LVITEM		LvItem;
            HWND		hList;
            ItrImports	itr;
            DWORD_PTR Index=0,i,len,lenght;
            char  Api[128],temp[128],*ptr,*String;
            
            hList=GetDlgItem(hWnd,IDC_DATA_LIST);
            // Add Columns
            memset(&LvCol,0,sizeof(LvCol));  // Init the struct to ZERO
            LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
            LvCol.pszText="Defined Name";
            LvCol.cx=0x70;
            SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); 
            LvCol.pszText="String";
            LvCol.cx=0x100;
            SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
            memset(&LvItem,0,sizeof(LvItem));	// Init the struct to ZERO
            LvItem.cchTextMax = 256;			// Max size of test
            
            if(DataSection.size()==0){// First Load
              itr=StrRefLines.begin();
              for(;itr!=StrRefLines.end();itr++,Index++){
                  i=(*itr); // Index in disasm window
                  LvItem.iItem=(DWORD)Index;
                  LvItem.iSubItem=0;
                  lstrcpy(temp,DisasmDataLines[i].GetComments());
                  ptr=temp;
                  while(*ptr!=':'){ // Pass the 'ASCIIZ' string
                      ptr++;
                  }
                  ptr+=3;
                  strcpy_s(Api,StringLen(ptr)+1,ptr); // Copy skipped string
                  len=StringLen(Api);
                  if(Api[len-1]==']'){
                      Api[len-1]=NULL;
                  }
                  Api[StringLen(Api)-3]=NULL; // terminate '"' char

                  // format spaces with '_'
				  lenght=StringLen(Api);
                  for(len=0;len<lenght;len++){
                      if(!
                          (
                           (Api[len]>=0x41 && Api[len]<=0x5A) ||
                           (Api[len]>=0x61 && Api[len]<=0x7A)
                          )
                        )
                          Api[len]='_';
                  }                 

				  if(StringLen(Api)>25){
                      Api[24]=NULL;
				  }
                  LvItem.pszText = Api; // string
                  LvItem.lParam = i;  // index in disasm window

                  String = new char[StringLen(Api)+1];
                  strcpy_s(String,StringLen(Api)+1,Api);
                  AddNewDataEntry((DWORD)i,String)

                  String=NULL;
                  LvItem.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE; // state
                  SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
                  LvItem.mask=LVIF_TEXT;

                  ptr--;
                  strcpy_s(Api,StringLen(ptr)+1,ptr);
                  len=StringLen(Api);
                  
                  if(Api[len-1]==']'){
                      Api[len-1]=NULL;
                  }

                  LvItem.iItem=(DWORD)Index;
                  LvItem.iSubItem=1;
                  LvItem.pszText=Api;
                  SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);
                  SendDlgItemMessage(hWnd,IDC_REF_LIST,LB_ADDSTRING,0,(LPARAM)Api);
                  // attach item number as data to the text
                  SendDlgItemMessage(hWnd,IDC_REF_LIST,LB_SETITEMDATA,(WPARAM)Index,(LPARAM)i);
              }
            }
            else{
                // Load saved at memory, do not forget to delete allocated memory
                // at (*DItr).second in the end!!!!
                ItrDataMap DItr=DataSection.begin();
                itr=StrRefLines.begin();
                for(;DItr!=DataSection.end();DItr++,itr++,Index++){
                    LvItem.iItem=(DWORD)Index;
                    LvItem.iSubItem=0;
                    LvItem.pszText = (*DItr).second; // string
                    LvItem.lParam = (*DItr).first;  // index in disasm window
                    LvItem.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE; // state
                    SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
                    LvItem.mask=LVIF_TEXT;
                    i=LvItem.lParam;// Index in disasm window
                    lstrcpy(temp,DisasmDataLines[i].GetComments());
                    ptr=temp;
                    while(*ptr!=':'){ // Pass the 'ASCIIZ' string
                        ptr++;
                    }
                    ptr+=2;
                    strcpy_s(Api,StringLen(ptr)+1,ptr); // Copy skiped string
                    int lenght=StringLen(Api);
                    if(Api[lenght-1]==']'){
                        Api[lenght-1]=NULL;
                    } 
                    LvItem.iItem=(DWORD)Index;
                    LvItem.iSubItem=1;
                    LvItem.pszText=Api;
                    SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);                         
                    SendDlgItemMessage(hWnd,IDC_REF_LIST,LB_ADDSTRING,0,(LPARAM)Api);
                    // attach item number as data to the text
                    SendDlgItemMessage(hWnd,IDC_REF_LIST,LB_SETITEMDATA,(WPARAM)Index,(LPARAM)i);
                }
            }
        }
		break;
        
		case WM_PAINT:{
		   return FALSE;
		}
		break;

        case WM_NOTIFY:{
            switch(LOWORD(wParam)){
                case IDC_DATA_LIST:{
                    LPNMLISTVIEW  pnm = (LPNMLISTVIEW)lParam;
                    LPNMLVKEYDOWN key = (LPNMLVKEYDOWN)lParam;

                    if(((LPNMHDR)lParam)->code == NM_DBLCLK){
                        DWORD_PTR iSelect=SendMessage(GetDlgItem(hWnd,IDC_DATA_LIST),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
                        if(iSelect==-1){
                            break;
                        }
                        ItrDataMap DItr=DataSection.begin();
                        for(DWORD_PTR i=0;i<iSelect;i++){
                            DItr++;
                        }
                        SetWindowText(GetDlgItem(hWnd,IDC_RENAME_EDIT),(*DItr).second);
                    }
					
                }
                break;
            }
        }
        break;
		
        case WM_COMMAND:{
            switch(LOWORD(wParam)){

				case IDC_WIZARD_DATA_BACK:{
                    EndDialog(hWnd,0);
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD), hWnd, (DLGPROC)WizardDlgProc);
 				}
				break;

				case IDC_DATA_DELETE:{
                    DWORD_PTR iSelect=SendMessage(GetDlgItem(hWnd,IDC_DATA_LIST),LVM_GETNEXTITEM,-1,LVNI_FOCUSED|LVNI_SELECTED);
                    if(iSelect==-1){
                        break;
                    }

					if(DataSection.size()!=0){
						ItrDataMap DItr=DataSection.begin();
						for(int i=0;i<iSelect;i++){
							DItr++;
						}

						delete[] (*DItr).second;
						DataSection.erase(DItr);
						ListView_DeleteItem(GetDlgItem(hWnd,IDC_DATA_LIST),iSelect);
					}
				}
				break;

                case IDC_DATA_RENAME:{
                    LVITEM LvItem;
                    char text[256];
                    GetWindowText(GetDlgItem(hWnd,IDC_RENAME_EDIT),text,256);
                    int len=StringLen(text);
                    
                    if(len==0)
                        break;

                    DWORD_PTR iSelect=SendMessage(GetDlgItem(hWnd,IDC_DATA_LIST),LVM_GETNEXTITEM,-1,LVNI_FOCUSED|LVNI_SELECTED);
                    if(iSelect==-1){
                        break;
                    }
                    
                    ItrDataMap DItr=DataSection.begin();
                    for(DWORD_PTR i=0;i<iSelect;i++){
                        DItr++;
                    }

                    memset(&LvItem,0,sizeof(LvItem));	// Init the struct to ZERO
                    LvItem.cchTextMax = 256;			// Max size of test  
                    LvItem.iItem=(DWORD)iSelect;
                    LvItem.iSubItem=0;
                    LvItem.pszText = text; // string
                    LvItem.lParam = (*DItr).first;  // index in disasm window
                    delete[] (*DItr).second;		// Free old String
                    (*DItr).second = new char[len+1];
                    lstrcpy((*DItr).second,text);
                    LvItem.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE; // state
                    SendMessage(GetDlgItem(hWnd,IDC_DATA_LIST),LVM_SETITEM ,0,(LPARAM)&LvItem);
                    ListView_Update(GetDlgItem(hWnd,IDC_DATA_LIST),iSelect);
                }
                break;

                case IDC_WIZARD_DATA_NEXT:{
                    EndDialog(hWnd,0);
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD_CODE), hWnd, (DLGPROC)WizardCodeDlgProc);                    
                }
                break;
            }
        }
        break;

        case WM_CLOSE:{ // We colsing the Dialog
          EndDialog(hWnd,0);
        }
	    break;
	}
	return 0;
}


BOOL CALLBACK WizardCodeDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: 
		{	
            LV_COLUMN	LvCol;
            LVITEM		LvItem;
            HWND		hList;
            char		temp[128];

            hList=GetDlgItem(hWnd,IDC_SUB_LIST);
            VarCount=1;
            FirstParam=TRUE;

            // Add Columns
            memset(&LvCol,0,sizeof(LvCol));  // Init the struct to ZERO
            LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
            LvCol.pszText="Assembly";
            LvCol.cx=0x110;
            SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);
            LvCol.pszText="Comments";
            LvCol.cx=0x90;
            SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
            memset(&LvItem,0,sizeof(LvItem));	// Init the struct to ZERO
            LvItem.cchTextMax = 256;			// Max size of test

            SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_ADDSTRING ,0,(LPARAM)"Select Range");
            SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_SETITEMDATA ,(WPARAM)0,(LPARAM)-1);

			for(int i=0,j=1;i<fFunctionInfo.size();i++,j++){
				if(lstrcmp(fFunctionInfo[i].FunctionName,"")==0){
					wsprintf(temp,"%08X - %08X (Main)",fFunctionInfo[i].FunctionStart,fFunctionInfo[i].FunctionEnd);
				}
				else{
					wsprintf(temp,"%08X - %08X (%s)",fFunctionInfo[i].FunctionStart,fFunctionInfo[i].FunctionEnd,fFunctionInfo[i].FunctionName);
				}

				SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_ADDSTRING ,0,(LPARAM)&temp);
                SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_SETITEMDATA ,(WPARAM)j,(LPARAM)j-1);
			}
            
            SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_SETCURSEL ,0,0);

            // add types
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"NO PARAMS");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"WORD");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"DWORD");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"HWND");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"UINT");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"WPARAM");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"LPARAM");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"BOOL");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_ADDSTRING ,0,(LPARAM)"OTHER");
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)0,(LPARAM)-1);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)1,(LPARAM)0);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)2,(LPARAM)1);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)3,(LPARAM)2);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)4,(LPARAM)3);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)5,(LPARAM)4);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)6,(LPARAM)5);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)7,(LPARAM)6);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETITEMDATA ,(WPARAM)8,(LPARAM)7);
            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETCURSEL ,0,0);
            EnableWindow(GetDlgItem(hWnd,IDC_PROTO_EDIT),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_PROC_EDIT),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_CREATE_PROTO),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_ADD_TYPE),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_PARAM_OTHER),FALSE);
		}
		break;

		case WM_PAINT:{
		   return FALSE;
		}
		break;
		
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_WIZARD_CODE_BACK:{
                    EndDialog(hWnd,0);
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD_DATA), hWnd, (DLGPROC)WizardDataDlgProc);
                }
                break;

                case IDC_WIZARD_CODE_NEXT:{
                    EndDialog(hWnd,0);
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD_HEADER), hWnd, (DLGPROC)WizardHeaderDlgProc);

                }
                break;

				case IDC_CREATE_EMPTY:{
					SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),"");
					SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),"");
				}
				break;

				case IDC_AUTO_SEARCH_CREATE:{ // Create and define functions automatically.
					// Automatic search and defines code here
					//MessageBox(hWnd,"test","test",MB_OK);
				}
				break;

				case IDC_CREATE_IMPORT:{ // import functions from a xpr file.
                    CODE_SECTION	codeSection;
					OPENFILENAME	ofn;
					HANDLE			hFile;
					DWORD			j=0,k,RetVal,WritenBytes,temp,size=0;
					char			FileName[MAX_PATH]="",data[512];

					// Inialuize File Browser Struct
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of Struct
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = "Export Files (*.xpr)\0*.xpr\0"; // File Filter
					ofn.lpstrFile=FileName;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt="xpr";

                    if(GetOpenFileName(&ofn)!=NULL){
                        hFile = CreateFile(FileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
                        if(hFile==INVALID_HANDLE_VALUE){						
                            return false;
                        }
                        
                        // load size
                        ReadFile(hFile,&size,sizeof(DWORD),&WritenBytes,NULL);
                        j=0;
                        do{ // read the file
                            RetVal=ReadFile(hFile,&data[j],sizeof(char),&WritenBytes,NULL);
                            j++;
                        }while(data[j-1]!=0x0A);
                        
                        for(DWORD_PTR i=0;i<size;i++){ // iterate file's size
                            for(k=0;k<6;k++){
                                switch(k+1){   
                                    case 1: case 2: case 3:{
                                        j=0;
                                        do{
                                            RetVal=ReadFile(hFile,&data[j],sizeof(char),&WritenBytes,NULL);
                                            // Did We Read beyond the File's End?
											if(WritenBytes==NULL && RetVal!=0){
                                                break;
											}
                                            j++;
                                        }
                                        while(data[j-1]!=0x0A); // Read a line
                                        data[j-2]='\0';
                                        //strcpy(data,"");
                                    }
                                    break;
                                    
                                    case 4: case 5: case 6:{
                                        ReadFile(hFile,&temp,sizeof(DWORD),&WritenBytes,NULL);
                                        j=0;
                                        do{
                                            RetVal=ReadFile(hFile,&data[j],sizeof(char),&WritenBytes,NULL);
                                            j++;
                                        }while(data[j-1]!=0x0A);
                                    }
                                    break;
                                }
                                
                                switch(k+1){
                                    case 1:{ 
                                        codeSection.Proc=new char[StringLen(data)+1]; 
                                        lstrcpy(codeSection.Proc,data);
                                    } 
                                    break;
                                    
                                    case 2:{
                                        codeSection.Proto=new char[StringLen(data)+1]; 
                                        lstrcpy(codeSection.Proto,data);
                                    } 
                                    break;
                                    
                                    case 3:{
                                        codeSection.Endp=new char[StringLen(data)+1]; 
                                        lstrcpy(codeSection.Endp,data);
                                    }
                                    break;
                                    
                                    case 4: { codeSection.Start = temp; } break;
                                    case 5: { codeSection.End = temp;   } break;
                                    case 6: { codeSection.Index = temp; } break;
                                }
                            }   

                            CodeSections.insert(CodeSections.end(),codeSection); // insert
                        }
                        CloseHandle(hFile);
                    }
				}
				break;

				case IDC_CREATE_EXPORT:{ // Export a xpr file
					OPENFILENAME ofn;
					HANDLE hFile;
					DWORD_PTR Size,i;
					DWORD WritenBytes;
					char FileName[MAX_PATH]="",*data=NULL;
					
					ZeroMemory(&ofn, sizeof(OPENFILENAME));
					ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of Struct
					ofn.hwndOwner = hWnd;
					ofn.lpstrFilter = "Export Files (*.xpr)\0*.xpr\0"; // File Filter
					ofn.lpstrFile=FileName;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					ofn.lpstrDefExt="xpr";

					if(GetSaveFileName(&ofn)){ // file save dialog
						hFile = CreateFile(FileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
						if(hFile==INVALID_HANDLE_VALUE){    
							MessageBox(hWnd,"Can't Create Export File!","Error Exporting",MB_OK);
							return FALSE;
						}

                        Size=CodeSections.size(); // number if functions to export
                        WriteFile(hFile,&Size,sizeof(Size),&WritenBytes,NULL);
                        WriteFile(hFile,"\r\n",2,&WritenBytes,NULL);

						for(i=0;i<Size;i++){
							if(data) delete data;

							data=new char[StringLen(CodeSections[i].Proc)+3];
							lstrcpy(data,CodeSections[i].Proc);
							lstrcat(data,"\r\n");
							WriteFile(hFile,data,StringLen(data),&WritenBytes,NULL);

							delete data;
							data=new char[StringLen(CodeSections[i].Proto)+3];
							lstrcpy(data,CodeSections[i].Proto);
							lstrcat(data,"\r\n");
							WriteFile(hFile,data,StringLen(data),&WritenBytes,NULL);

							delete data;
							data=new char[StringLen(CodeSections[i].Endp)+3];
							lstrcpy(data,CodeSections[i].Endp);
							lstrcat(data,"\r\n");
							WriteFile(hFile,data,StringLen(data),&WritenBytes,NULL);

							WriteFile(hFile,&CodeSections[i].Start,sizeof(DWORD),&WritenBytes,NULL);
							WriteFile(hFile,"\r\n",2,&WritenBytes,NULL);
							WriteFile(hFile,&CodeSections[i].End,sizeof(DWORD),&WritenBytes,NULL);
							WriteFile(hFile,"\r\n",2,&WritenBytes,NULL);
							WriteFile(hFile,&CodeSections[i].Index,sizeof(DWORD),&WritenBytes,NULL);
							WriteFile(hFile,"\r\n",2,&WritenBytes,NULL);
						}

						if(data) delete data;
						CloseHandle(hFile);
					}
				}
				break;

                case IDC_CREATE_AUTO:{ // Auto search
                    DWORD_PTR index,address,size,PushCounter,i;
                    BOOL FoundCall=FALSE;
                    char temp[256],*ptr;
                    index =  SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_GETCURSEL ,0,0);
                    index=SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_GETITEMDATA ,index,0);
                    
                    if(index!=-1){ // if selected function is not the main
                        // search for a caller address
                        address=fFunctionInfo[index].FunctionStart; 
                        size=DisasmCodeFlow.size();
                        for(i=0;i<size;i++){                        
							if(DisasmCodeFlow[i].Branch_Destination==address){
                                if(DisasmCodeFlow[i].BranchFlow.Call==TRUE){                                
                                    FoundCall=TRUE;
                                    break;
                                }
							}
                        }

                        if(FoundCall){
                            PushCounter=0;
                            index=DisasmCodeFlow[i].Current_Index;
                            index--; // point to the PUSH instructions
                            for(;;){                            
                                strcpy_s(temp,StringLen(DisasmDataLines[index].GetMnemonic())+1,DisasmDataLines[index].GetMnemonic());
                                ptr=temp;
                                while(*ptr!=' ')
                                    ptr++;
                                
                                *ptr=NULL;
                                if(lstrcmp(temp,"PUSH")==0 || lstrcmp(temp,"push")==0){
                                    PushCounter++;
                                    index--;
                                }
                                else break; // no more PUSHs
                            }
                        }
                        else return FALSE;
                        
                        if(PushCounter>0){
                            EnableWindow(GetDlgItem(hWnd,IDC_ADD_TYPE),TRUE);
                            SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_SETCURSEL,2,0);
                            for(i=0;i<PushCounter;i++)
                                AddType(hWnd);
                        }
                    }
                }
                break;

                case IDC_CREATE_PROTO:{
                    CODE_SECTION codeSection;
                    CodeSectionItr CItr;
					int i;
					bool Found=FALSE;
                    char Buffer[256],Buffer2[256];

                    ZeroMemory(&codeSection,sizeof(CODE_SECTION));
                    GetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),Buffer,256);
                    GetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),Buffer2,256);

                    if(StringLen(Buffer)==0 || StringLen(Buffer2)==0){
						MessageBox(hWnd,"Proc/Proto skeleton is empty\nCreating an Empty Definition...","Notice",MB_OK);                        
                    }

                    DWORD_PTR index =  SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_GETCURSEL ,0,0);  
                    index=SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_GETITEMDATA ,index,0);  
                    
                    if(index!=-1){
                        codeSection.Start=fFunctionInfo[index].FunctionStart;
                        codeSection.End = fFunctionInfo[index].FunctionEnd;
                        codeSection.Index=(DWORD)index;
                        codeSection.Proto = new char[StringLen(Buffer)+1];
                        codeSection.Proc= new char[StringLen(Buffer2)+1];
                        lstrcpy(codeSection.Proto,Buffer);
                        lstrcpy(codeSection.Proc,Buffer2);
                        i=0;
						if(StringLen(Buffer2)!=0){
							while(Buffer2[i]!=' '){
								i++;
							}
						}
                        Buffer[i]=NULL;
						if(StringLen(Buffer)!=0){						
							wsprintf(Buffer2,"%s endp",Buffer);
							codeSection.Endp = new char[StringLen(Buffer2)+1];
							lstrcpy(codeSection.Endp,Buffer2);
						}
						else{ // main
							codeSection.Endp = new char[1];
							lstrcpy(codeSection.Endp,"");
						}

                        CItr = CodeSections.begin();
                        if(CItr == CodeSections.end()){
                          CodeSections.insert(CodeSections.end(),codeSection);
                        }
                        else{
                            for(int i=0;i<CodeSections.size();i++){
                                if(
                                    CodeSections[i].Start==codeSection.Start &&
                                    CodeSections[i].End==codeSection.End
                                  )
                                {
                                    delete[]CodeSections[i].Proc;
                                    delete[]CodeSections[i].Proto;
                                    CodeSections[i].Proc = new char[StringLen(codeSection.Proc)+1];
                                    CodeSections[i].Proto = new char[StringLen(codeSection.Proto)+1];
                                    lstrcpy(CodeSections[i].Proc,codeSection.Proc);
                                    lstrcpy(CodeSections[i].Proto,codeSection.Proto);
                                    Found=TRUE;
                                    break;
                                }
                            }

                            if(Found==FALSE){
                                CodeSections.insert(CodeSections.end(),codeSection);
                            }
                        }
                    }
                }
                break;

                case IDC_FUNCTIONS_LIST:{
                    switch(HIWORD(wParam)){

                        case CBN_SELCHANGE:{ // display function info when a function is selected
							try{
							  LVITEM LvItem;
							  DWORD_PTR  Start=0,End,Address=0;
							  char   text[128];
							  HWND   hList = GetDlgItem(hWnd,IDC_SUB_LIST);
							  bool   Found=FALSE;
                          
							  VarCount=1;
							  FirstParam=TRUE; // on sel change, reset first param
							  SendMessage(hList,LVM_DELETEALLITEMS,0,0);
							  DWORD_PTR index =  SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_GETCURSEL ,0,0);  
							  index=SendMessage(GetDlgItem(hWnd,IDC_FUNCTIONS_LIST),CB_GETITEMDATA ,index,0);  
							  if(index!=-1){
								EnableWindow(GetDlgItem(hWnd,IDC_PROTO_EDIT),TRUE);
								EnableWindow(GetDlgItem(hWnd,IDC_PROC_EDIT),TRUE);
								EnableWindow(GetDlgItem(hWnd,IDC_CREATE_PROTO),TRUE);

								Start=fFunctionInfo[index].FunctionStart;
								End = fFunctionInfo[index].FunctionEnd;

								memset(&LvItem,0,sizeof(LvItem));	// Init the struct to ZERO   
								LvItem.mask=LVIF_TEXT;
								LvItem.cchTextMax = 256;			// Max size of test   

								for(int iLoop=0,iIndex=0;iLoop<DisasmDataLines.size();iLoop++){
									try{
										if(StringLen(DisasmDataLines[iLoop].GetAddress())==0){
											continue;
										}
									}
									catch(...){ // do nothing
									}

									Address=StringToDword(DisasmDataLines[iLoop].GetAddress());
									if(Address>End){
										break;
									}
									
									if(Address>=Start && Address<=End){
										LvItem.iItem=iIndex;
										LvItem.iSubItem=0;
										LvItem.pszText=DisasmDataLines[iLoop].GetMnemonic();
										SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
										LvItem.iSubItem=1;
										LvItem.pszText=DisasmDataLines[iLoop].GetComments();
										SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);
										iIndex++;

										if(CodeSections.size()==0){
											if( lstrcmp(fFunctionInfo[index].FunctionName,"")==0){
												wsprintf(text,"proc_%08X proto",Start);
												SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),text);
												wsprintf(text,"proc_%08X proc",Start);
												SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),text);
											}
											else{
												wsprintf(text,"%s proto",fFunctionInfo[index].FunctionName);
												SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),text);
												wsprintf(text,"%s proc",fFunctionInfo[index].FunctionName);
												SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),text);
											}
										}
										else{
											for(int i=0;i<CodeSections.size();i++){
												if(CodeSections[i].Index==index){
													SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),CodeSections[i].Proto);
													SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),CodeSections[i].Proc);
													Found=TRUE;
													break;
												}
											}

											if(Found==FALSE){
												if(lstrcmp(fFunctionInfo[index].FunctionName,"")==0){
													wsprintf(text,"proc_%08X proto",Start);
													SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),text);
													wsprintf(text,"proc_%08X proc",Start);
													SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),text);
												}
												else{
													wsprintf(text,"%s proto",fFunctionInfo[index].FunctionName);
													SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),text);
													wsprintf(text,"%s proc",fFunctionInfo[index].FunctionName);
													SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),text);
												}
											}
										}
									}
                                
								}
							  }
							  else {
								  SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),"");
								  SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),"");
								  EnableWindow(GetDlgItem(hWnd,IDC_PROTO_EDIT),FALSE);
								  EnableWindow(GetDlgItem(hWnd,IDC_PROC_EDIT),FALSE);
								  EnableWindow(GetDlgItem(hWnd,IDC_CREATE_PROTO),FALSE);
							  }
						  }
						  catch(...){
							  MessageBox(hWnd,"Error In Function Code Builder.","Error",MB_OK);
						  }
                        }
                        break;
                    }
                }
                break;
                
                case IDC_PARAM_TYPE:{
                   switch(HIWORD(wParam)){

                     case CBN_SELCHANGE:{
                         DWORD_PTR index =  SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_GETCURSEL ,0,0);
                         index=SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_GETITEMDATA ,index,0);  
                         
                         if(index==-1){
                             EnableWindow(GetDlgItem(hWnd,IDC_ADD_TYPE),FALSE);
                             EnableWindow(GetDlgItem(hWnd,IDC_PARAM_OTHER),FALSE);
                             break;
                         }
                         
                         if(index>6){                                   
                             EnableWindow(GetDlgItem(hWnd,IDC_PARAM_OTHER),TRUE);
                         }
                         else EnableWindow(GetDlgItem(hWnd,IDC_PARAM_OTHER),FALSE);
                         
                         EnableWindow(GetDlgItem(hWnd,IDC_ADD_TYPE),TRUE);
                       }
                       break;
                    }
                 }
                 break;

                 case IDC_ADD_TYPE:{
                     AddType(hWnd);
                 }
                 break;
            }
        }        
        break;

        case WM_CLOSE:{ // We colsing the Dialog
          EndDialog(hWnd,0); 
        }
	    break;
	}
	return 0;
}


BOOL CALLBACK WizardHeaderDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG:{	
            HWND hComboMachine = GetDlgItem(hWnd,IDC_MACHINE);
            HWND hComboCase = GetDlgItem(hWnd,IDC_CASEMAP);
            
            SendMessage(hComboMachine,CB_ADDSTRING,0,(LPARAM)".386");
            SendMessage(hComboMachine,CB_SETITEMDATA,(WPARAM)0,(LPARAM)386);
            SendMessage(hComboMachine,CB_ADDSTRING,0,(LPARAM)".486");
            SendMessage(hComboMachine,CB_SETITEMDATA,(WPARAM)1,(LPARAM)486);
            SendMessage(hComboMachine,CB_ADDSTRING,0,(LPARAM)".586");
            SendMessage(hComboMachine,CB_SETITEMDATA,(WPARAM)2,(LPARAM)586);
            SendMessage(hComboMachine,CB_SETCURSEL,0,0);

            SendMessage(hComboCase,CB_ADDSTRING,0,(LPARAM)"None");
            SendMessage(hComboCase,CB_SETITEMDATA,(WPARAM)0,(LPARAM)0);
            SendMessage(hComboCase,CB_ADDSTRING,0,(LPARAM)"All");
            SendMessage(hComboCase,CB_SETITEMDATA,(WPARAM)1,(LPARAM)1);
            SendMessage(hComboCase,CB_ADDSTRING,0,(LPARAM)"NotPublic");
            SendMessage(hComboCase,CB_SETITEMDATA,(WPARAM)2,(LPARAM)2);
            SendMessage(hComboCase,CB_SETCURSEL,0,0);

            SetWindowText(GetDlgItem(hWnd,IDC_EP_NAME),"start");

            if(WizardOptions.SavedOptions==FALSE){
                SendMessage(GetDlgItem(hWnd,IDC_MODEL_STDCALL),BM_SETCHECK,BST_CHECKED,0);
                SendMessage(GetDlgItem(hWnd,IDC_MODEL_FLAT),BM_SETCHECK,BST_CHECKED,0);
                SendMessage(GetDlgItem(hWnd,IDC_WIZARD_LIBS),BM_SETCHECK,BST_CHECKED,0);
                SendMessage(GetDlgItem(hWnd,IDC_WIZARD_INCLUDE),BM_SETCHECK,BST_CHECKED,0); 
                SendMessage(GetDlgItem(hWnd,IDC_PREV_COMMENTS),BM_SETCHECK,BST_CHECKED,0); 
                EnableWindow(GetDlgItem(hWnd,IDC_WIZARD_HEADER_NEXT),FALSE);
            }
            else{
                LoadOptions(hWnd);
                EnableWindow(GetDlgItem(hWnd,IDC_WIZARD_HEADER_NEXT),TRUE);
            }
		}
		break;

		case WM_PAINT:{			
		   return FALSE;				
		}
		break;
		
        case WM_COMMAND:{
            switch(LOWORD(wParam)){	

                case IDC_WIZARD_HEADER_BACK:{
                  EndDialog(hWnd,0);
                  DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD_CODE), hWnd, (DLGPROC)WizardCodeDlgProc);
                }
                break;

                case IDC_SET_OPTIONS:{
                    DWORD_PTR Data;
                    char Entry[128];

                    Data = SendMessage(GetDlgItem(hWnd,IDC_MACHINE),CB_GETCURSEL ,0,0);
                    Data = SendMessage(GetDlgItem(hWnd,IDC_MACHINE),CB_GETITEMDATA ,Data,0);

                    // MACHINE TYPE
                    WizardOptions.Machine = Data;
                    Data = SendMessage(GetDlgItem(hWnd,IDC_CASEMAP),CB_GETCURSEL ,0,0);
                    Data = SendMessage(GetDlgItem(hWnd,IDC_CASEMAP),CB_GETITEMDATA ,Data,0);
                    // CASEMAP
                    WizardOptions.CaseMap = Data;
                    GetWindowText(GetDlgItem(hWnd,IDC_EP_NAME),Entry,128);
                    // ENTRYPOINT NAME
                    if(WizardOptions.EntryName==NULL){
                        WizardOptions.EntryName = new char[StringLen(Entry)+1];
                        lstrcpy(WizardOptions.EntryName,Entry);
                    }
                    else{
                        delete[]WizardOptions.EntryName;
                        WizardOptions.EntryName = new char[StringLen(Entry)+1];
                        lstrcpy(WizardOptions.EntryName,Entry);
                    }

                    //CALLING CONVERTIONS
                    if(SendDlgItemMessage(hWnd,IDC_MODEL_SYSCALL,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Convertion=SYSCALL;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_C,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Convertion=C_CALL;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_PASCAL,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Convertion=PASCAL_C;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_STDCALL,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Convertion=STDCALL;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_FORTRAN,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Convertion=FORTRAN;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_BASIC,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Convertion=BASIC;
                    }

                    // MEMORY MODEL
                    if(SendDlgItemMessage(hWnd,IDC_MODEL_TINY,BM_GETCHECK,0,0)==BST_CHECKED)
                    {
                        WizardOptions.Model =TINY;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_MEDIUM,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Model=MEDIUM;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_COMPACT,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Model=COMPACT;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_SMALL,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Model=SMALL;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_LARGE,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Model=LARGE;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_HUGE,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Model=HUGE;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_MODEL_FLAT,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Model=FLAT;
                    }

                    // DIRECTIVES
                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_NOLIST,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Directive|=0x20;
                    }
                    else{
                        WizardOptions.Directive&=~0x20;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_MMX,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Directive|=0x8;
                    }
                    else{
                        WizardOptions.Directive&=~0x8;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_LIST,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Directive|=0x2;
                    }
                    else{
                        WizardOptions.Directive&=~0x2;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_NOLISTMACRO,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Directive|=0x10;
                    }
                    else{
                        WizardOptions.Directive&=~0x10;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_NOLISTIF,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Directive|=0x4;
                    }
                    else{
                        WizardOptions.Directive&=~0x4;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_LALL,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Directive|=0x1;
                    }
                    else{
                        WizardOptions.Directive&=~0x1;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_LIBS,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Libs=TRUE;
                    }
                    else{
                        WizardOptions.Libs=FALSE;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_WIZARD_INCLUDE,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.Include=TRUE;
                    }
                    else{
                        WizardOptions.Include=FALSE;
                    }

                    if(SendDlgItemMessage(hWnd,IDC_PREV_COMMENTS,BM_GETCHECK,0,0)==BST_CHECKED){
                        WizardOptions.AddComments=TRUE;
                    }
                    else{
                        WizardOptions.AddComments=FALSE;
                    }

                    WizardOptions.SavedOptions=TRUE;
                    EnableWindow(GetDlgItem(hWnd,IDC_WIZARD_HEADER_NEXT),TRUE);
                    
                }
                break;

                case IDC_WIZARD_HEADER_NEXT:{
                    EndDialog(hWnd,0);
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD_PREVIEW), hWnd, (DLGPROC)WizardPreviewDlgProc);

                }
                break;
            }
        }
        break;

        case WM_CLOSE:{ // We colsing the Dialog
          EndDialog(hWnd,0); 
        }
	    break;
	}
	return 0;
}

void LoadOptions(HWND hWnd)
{
  switch(WizardOptions.Machine)
  {
    case 386: SendMessage(GetDlgItem(hWnd,IDC_MACHINE),CB_SETCURSEL,0,0); break;
    case 486: SendMessage(GetDlgItem(hWnd,IDC_MACHINE),CB_SETCURSEL,1,0); break;
    case 586: SendMessage(GetDlgItem(hWnd,IDC_MACHINE),CB_SETCURSEL,2,0); break;
    default:  SendMessage(GetDlgItem(hWnd,IDC_MACHINE),CB_SETCURSEL,0,0);
  }

  switch(WizardOptions.CaseMap) {
    case 0: SendMessage(GetDlgItem(hWnd,IDC_CASEMAP),CB_SETCURSEL,0,0); break;
    case 1: SendMessage(GetDlgItem(hWnd,IDC_CASEMAP),CB_SETCURSEL,1,0); break;
    case 2: SendMessage(GetDlgItem(hWnd,IDC_CASEMAP),CB_SETCURSEL,2,0); break;
    default:SendMessage(GetDlgItem(hWnd,IDC_CASEMAP),CB_SETCURSEL,0,0);
  }

  SetWindowText(GetDlgItem(hWnd,IDC_EP_NAME),WizardOptions.EntryName);

  switch(WizardOptions.Model) {
    case TINY:    SendMessage(GetDlgItem(hWnd,IDC_MODEL_TINY),BM_SETCHECK,BST_CHECKED,0); break;
    case MEDIUM:  SendMessage(GetDlgItem(hWnd,IDC_MODEL_MEDIUM),BM_SETCHECK,BST_CHECKED,0);       break;
    case COMPACT: SendMessage(GetDlgItem(hWnd,IDC_MODEL_COMPACT),BM_SETCHECK,BST_CHECKED,0);  break;
    case SMALL:   SendMessage(GetDlgItem(hWnd,IDC_MODEL_SMALL),BM_SETCHECK,BST_CHECKED,0); break;
    case LARGE:   SendMessage(GetDlgItem(hWnd,IDC_MODEL_LARGE),BM_SETCHECK,BST_CHECKED,0); break;
    case HUGE:    SendMessage(GetDlgItem(hWnd,IDC_MODEL_HUGE),BM_SETCHECK,BST_CHECKED,0);   break;
    case FLAT:    SendMessage(GetDlgItem(hWnd,IDC_MODEL_FLAT),BM_SETCHECK,BST_CHECKED,0);   break;
    default:      SendMessage(GetDlgItem(hWnd,IDC_MODEL_FLAT),BM_SETCHECK,BST_CHECKED,0); break;
  }

  switch(WizardOptions.Convertion) {
    case SYSCALL:	SendMessage(GetDlgItem(hWnd,IDC_MODEL_SYSCALL),BM_SETCHECK,BST_CHECKED,0); break;
    case C_CALL:	SendMessage(GetDlgItem(hWnd,IDC_MODEL_C),BM_SETCHECK,BST_CHECKED,0);       break;
    case PASCAL_C:	SendMessage(GetDlgItem(hWnd,IDC_MODEL_PASCAL),BM_SETCHECK,BST_CHECKED,0);  break;
    case STDCALL:	SendMessage(GetDlgItem(hWnd,IDC_MODEL_STDCALL),BM_SETCHECK,BST_CHECKED,0); break;
    case FORTRAN:	SendMessage(GetDlgItem(hWnd,IDC_MODEL_FORTRAN),BM_SETCHECK,BST_CHECKED,0); break;
    case BASIC:		SendMessage(GetDlgItem(hWnd,IDC_MODEL_BASIC),BM_SETCHECK,BST_CHECKED,0);   break;
    default:		SendMessage(GetDlgItem(hWnd,IDC_MODEL_STDCALL),BM_SETCHECK,BST_CHECKED,0); break;
  }

  if( (WizardOptions.Directive&0x1) == 1 )
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_LALL),BM_SETCHECK,BST_CHECKED,0);

  if( ((WizardOptions.Directive&0x2)>>1) == 1 )
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_LIST),BM_SETCHECK,BST_CHECKED,0);

  if( ((WizardOptions.Directive&0x4)>>2) == 1 )
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_NOLISTIF),BM_SETCHECK,BST_CHECKED,0);

  if( ((WizardOptions.Directive&0x8)>>3) == 1 )
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_MMX),BM_SETCHECK,BST_CHECKED,0);

  if( ((WizardOptions.Directive&0x10)>>4) == 1 )
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_NOLISTMACRO),BM_SETCHECK,BST_CHECKED,0);

  if( ((WizardOptions.Directive&0x20)>>5) == 1 )
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_NOLIST),BM_SETCHECK,BST_CHECKED,0);

  if(WizardOptions.Libs==TRUE)
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_LIBS),BM_SETCHECK,BST_CHECKED,0);

  if(WizardOptions.Include==TRUE)
      SendMessage(GetDlgItem(hWnd,IDC_WIZARD_INCLUDE),BM_SETCHECK,BST_CHECKED,0);
  
  if(WizardOptions.AddComments==TRUE)
      SendMessage(GetDlgItem(hWnd,IDC_PREV_COMMENTS),BM_SETCHECK,BST_CHECKED,0);
}

BOOL CALLBACK WizardPreviewDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG:{	
            SendMessage(GetDlgItem(hWnd,IDC_WIZARD_PREVIEW),LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT/*|LVS_EX_GRIDLINES*/|LVS_EX_FLATSB);
            PrevhWnd = hWnd;
            DWORD EntryPoint=nt_hdr->OptionalHeader.AddressOfEntryPoint;
            OEP=EntryPoint+nt_hdr->OptionalHeader.ImageBase;
            MakePreview(hWnd);
		}
		break;
        
        case WM_NOTIFY: // Notify Messages
        {
            switch(LOWORD(wParam)){
                //Disassembler Control (Virtual List View)
                case IDC_WIZARD_PREVIEW:{  // Disassembly ListView Messages
                    LPNMLISTVIEW pnm		= (LPNMLISTVIEW)lParam;
                    HWND         hDisasm	= GetDlgItem(hWnd,IDC_WIZARD_PREVIEW);
                    LV_DISPINFO* nmdisp		= (LV_DISPINFO*)lParam;
                    LV_ITEM*     pItem		= &(nmdisp)->item;

                    // Change Window Style on Custom Draw
                    if(pnm->hdr.code == NM_CUSTOMDRAW){
                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, (LONG_PTR)ProcessWizardCustomDraw(lParam));
                        return TRUE;
                    }
                    
                    // Set the information to the list view
                    if(((LPNMHDR)lParam)->code==LVN_GETDISPINFO){   
                      lstrcpy(WizardBuffer, WizardCodeList[nmdisp->item.iItem].SourceCode);
                      nmdisp->item.pszText=WizardBuffer; // item 0
                    }

                    // Update/keep The information on the list view
                    if(((LPNMHDR)lParam)->code==LVN_SETDISPINFO){
                        lstrcpy(WizardBuffer, WizardCodeList[nmdisp->item.iItem].SourceCode);
                        nmdisp->item.pszText=WizardBuffer; // Item 0
                    }
                }
                break;
            }
        }
        break;

		case WM_PAINT:
		{
		   return FALSE;
		}
		break;
		
        case WM_COMMAND:{
            switch(LOWORD(wParam)){

                case IDC_WIZARD_PREV_BACK:{
                  EndDialog(hWnd,0);
                  DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD_HEADER), hWnd, (DLGPROC)WizardHeaderDlgProc);
                }
                break;

				case IDC_WIZARD_PREV_CLOSE:{
					SendMessage(hWnd,WM_CLOSE,0,0);
				}
				break;

                case IDC_WIZARD_BUILD:{
                    if(StringLen(prjFileName)==0){
                        MessageBox(hWnd,"Can't Build project, Specify a Project Name!","Notice",MB_OK|MB_ICONEXCLAMATION);
                        break;
                    }
                    hDisasmThread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)BuildSourceCode,hWnd,0,&DisasmThreadId);
                }
                break;
            }
        }
        break;

        case WM_CLOSE:{ // We colsing the Dialog
          EndDialog(hWnd,0);
        }
	    break;
	}
	return 0;
}

LRESULT ProcessWizardCustomDraw (LPARAM lParam)
{
    LPNMLVCUSTOMDRAW	lplvcd = (LPNMLVCUSTOMDRAW)lParam;
    LV_DISPINFO*		nmdisp = (LV_DISPINFO*)lParam;
    LV_ITEM*			pItem= &(nmdisp)->item;
    
    switch(lplvcd->nmcd.dwDrawStage){
        case CDDS_PREPAINT: //Before the paint cycle begins
        //request notifications for individual listview items
        return CDRF_NOTIFYITEMDRAW;
        
        case CDDS_ITEMPREPAINT: //Before an item is drawn
        {
            return CDRF_NOTIFYSUBITEMDRAW;
        }
        break;
        
        // Paint the List View's Items
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT:{ //Before a subitem is drawn
            switch(lplvcd->iSubItem){
				// Color the Address Field
                case 0:{
                    lplvcd->clrText   = RGB(0,0,0);
                    lplvcd->clrTextBk = RGB(255,255,255);
                    return CDRF_NEWFONT;
                }
                break;
            }
        }
    }
    return CDRF_DODEFAULT;
}


void MakePreview(HWND hWnd)
{
	WIZARD_BUILD	Build;
    LV_COLUMN		LvCol;
    LVITEM			LvItem;
    HWND			hList;    
    DWORD			Index=0,ProcFrame=1;
	ItrDataMap		DataItr;
    bool			FoundJump=FALSE,FoundRef=FALSE,FoundData=FALSE;
	char			Temp[128],Temp2[128],*ptr;

    if(WizardCodeList.size()!=0){
            WizardCodeList.clear();
            //DisasmCodeFlow.swap(CodeBranch(DisasmCodeFlow));
            WizardList(WizardCodeList).swap(WizardCodeList);
    }

    hList=GetDlgItem(hWnd,IDC_WIZARD_PREVIEW);
    
    memset(&LvCol,0,sizeof(LvCol));  // Init the struct to ZERO
    LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
    LvCol.pszText="Assembly Source - Masm";
    LvCol.cx=0x200;
    SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); 
    
    memset(&LvItem,0,sizeof(LvItem)); // Init the struct to ZERO
    LvItem.cchTextMax = 256;          // Max size of test

    Build.Index=Index;
    lstrcpy(Build.SourceCode,HEADER_TAG);
    AddString(Build)
    Index++;

    // Tag Name
    Build.Index=Index;
    lstrcpy(Build.SourceCode,REL_TAG);
    AddString(Build)
    Index++;

    Build.Index=Index;
    lstrcpy(Build.SourceCode,REL_TAG2);
    AddString(Build)
    Index++;

    Build.Index=Index;
    lstrcpy(Build.SourceCode,HEADER_TAG);
    AddString(Build)
    Index++;

    wsprintf(Temp,".%d   ; create 32 bit code",WizardOptions.Machine);  
    Build.Index=Index;    
    lstrcpy(Build.SourceCode,Temp);
    AddString(Build)
    Index++;

    if( (WizardOptions.Directive&0x1) == 1 )
    {
        Build.Index=Index;
        lstrcpy(Build.SourceCode,".lall");
        AddString(Build);
        Index++;
    }
    
    if( ((WizardOptions.Directive&0x2)>>1) == 1 ){
        Build.Index=Index;
        lstrcpy(Build.SourceCode,".list");
        AddString(Build);
        Index++;
    }
    
    if( ((WizardOptions.Directive&0x4)>>2) == 1 ){
        Build.Index=Index;
        lstrcpy(Build.SourceCode,".nolistif");
        AddString(Build);
        Index++;
    }
    
    if( ((WizardOptions.Directive&0x8)>>3) == 1 ){
        Build.Index=Index;
        lstrcpy(Build.SourceCode,".mmx");
        AddString(Build);
        Index++;
    }
    
    if( ((WizardOptions.Directive&0x10)>>4) == 1 ){
        Build.Index=Index;
        lstrcpy(Build.SourceCode,".nolistmacro");
        AddString(Build);
        Index++;
    }
    
    if( ((WizardOptions.Directive&0x20)>>5) == 1 ){
        Build.Index=Index;
        lstrcpy(Build.SourceCode,".nolist");
        AddString(Build);
        Index++;
    }

    switch(WizardOptions.Model) {
        case TINY:		strcpy_s(Temp2,"tiny"); break;
        case MEDIUM:	strcpy_s(Temp2,"medium"); break;
        case COMPACT:	strcpy_s(Temp2,"compact"); break;
        case SMALL:		strcpy_s(Temp2,"small"); break;
        case LARGE:		strcpy_s(Temp2,"large"); break;
        case HUGE:		strcpy_s(Temp2,"huge"); break;
        case FLAT:		strcpy_s(Temp2,"flat"); break;
        default:		strcpy_s(Temp2,"flat"); 
    }

    wsprintf(Temp,".model %s, ",Temp2);

    switch(WizardOptions.Convertion) {
        case SYSCALL:	strcpy_s(Temp2,"syscall");break;
        case C_CALL:	strcpy_s(Temp2,"c");break;
        case PASCAL_C:	strcpy_s(Temp2,"pascal");break;
        case STDCALL:	strcpy_s(Temp2,"stdcall");break;
        case FORTRAN:	strcpy_s(Temp2,"fortran");break;
        case BASIC:		strcpy_s(Temp2,"basic");break;
        default:		strcpy_s(Temp2,"stdcall");
    }

    lstrcat(Temp,Temp2);
	if(WizardOptions.Model==FLAT){
        lstrcat(Temp," ; 32 bit memory model");
	}
    Build.Index=Index;
    lstrcpy(Build.SourceCode,Temp);
    AddString(Build);
    Index++;

    switch(WizardOptions.CaseMap){
        case 0: strcpy_s(Temp2,"none"); break;
        case 1: strcpy_s(Temp2,"all"); break;
        case 2: strcpy_s(Temp2,"notpublic"); break;
        default:strcpy_s(Temp2,"none");
    }

    wsprintf(Temp,"option casemap:%s  ; case sensitive",Temp2);
    Build.Index=Index;
    lstrcpy(Build.SourceCode,Temp);
    AddString(Build);
    Index++;

    Build.Index=Index;
    lstrcpy(Build.SourceCode,"");
    AddString(Build);
    Index++;

    if(WizardOptions.Include==TRUE){
        // fix masm path
        Build.Index=Index;
        wsprintf(Build.SourceCode,"include %s",MasmPath);
        lstrcat(Build.SourceCode,"\\include\\windows.inc");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"include %s",MasmPath);
        lstrcat(Build.SourceCode,"\\include\\kernel32.inc");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"include %s",MasmPath);
        lstrcat(Build.SourceCode,"\\include\\user32.inc");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"include %s",MasmPath);
        lstrcat(Build.SourceCode,"\\include\\gdi32.inc");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"include %s",MasmPath);
        lstrcat(Build.SourceCode,"\\include\\comctl32.inc");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"include %s",MasmPath);
        lstrcat(Build.SourceCode,"\\include\\shell32.inc");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"include %s",MasmPath);
        lstrcat(Build.SourceCode,"\\include\\comdlg32.inc");
        AddString(Build);
        Index++;
		
		Build.Index=Index;
		lstrcpy(Build.SourceCode,"");
		AddString(Build);
		Index++;
    }

    if(WizardOptions.Libs==TRUE){
        Build.Index=Index;
        wsprintf(Build.SourceCode,"includelib %s",MasmPath);
        lstrcat(Build.SourceCode,"\\lib\\user32.lib");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"includelib %s",MasmPath);
        lstrcat(Build.SourceCode,"\\lib\\kernel32.lib");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"includelib %s",MasmPath);
        lstrcat(Build.SourceCode,"\\lib\\gdi32.lib");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"includelib %s",MasmPath);
        lstrcat(Build.SourceCode,"\\lib\\comctl32.lib");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"includelib %s",MasmPath);
        lstrcat(Build.SourceCode,"\\lib\\shell32.lib");
        AddString(Build);
        Index++;

        Build.Index=Index;
        wsprintf(Build.SourceCode,"includelib %s",MasmPath);
        lstrcat(Build.SourceCode,"\\lib\\comdlg32.lib");
        AddString(Build);
        Index++;
    }

    Build.Index=Index;
    lstrcpy(Build.SourceCode,HEADER_TAG);
    AddString(Build)
    Index++;
    
    Build.Index=Index;
    lstrcpy(Build.SourceCode,"");
    AddString(Build);
    Index++;

    // PROTOTYPES
    if(fFunctionInfo.size()!=0){
        if(CodeSections.size()!=0){
            for(int i=0;i<CodeSections.size();i++){
                if(StringLen(CodeSections[i].Proto)!=0){
                    Build.Index=Index;
                    lstrcpy(Build.SourceCode,CodeSections[i].Proto);
                    AddString(Build);
                    Index++;
                }
            }

        }
    }
    
    Build.Index=Index;
    lstrcpy(Build.SourceCode,"");
    AddString(Build);
    Index++;

    // Create the DATA secgment
    if(DataSection.size()!=0){
        Build.Index=Index;    
        lstrcpy(Build.SourceCode,".data");
        AddString(Build);
        Index++;
        DataItr = DataSection.begin();
        int len,Dindex;

        for(;DataItr!=DataSection.end();DataItr++){
			FoundData=FALSE;
            Dindex=(*DataItr).first;
            lstrcpy(Temp2,DisasmDataLines[Dindex].GetComments());
            if(StringLen(Temp2)!=0){
                ptr=Temp2;
				while(*ptr!=':'){ // Pass 'ASCIIZ'
                    ptr++;
				}
                ptr+=2;
                strcpy_s(Temp,StringLen(ptr)+1,ptr);
                len=StringLen(Temp);
                if(Temp[len-1]==']')
                    Temp[len-1]=NULL;
                
                wsprintf(Temp2,"%s db %s",(*DataItr).second,Temp);
				
				// Scan created .DATA and remove dupliactions
				for(DWORD sIndex=0;sIndex<WizardCodeList.size();sIndex++){ // check if already defined
					if(lstrcmp(WizardCodeList[sIndex].SourceCode,Temp2)==0){
						FoundData=TRUE;
						break;
					}
                }

				if(FoundData==TRUE){ // don't add this line
					continue;
				}

				lstrcpy(Build.SourceCode,Temp2);
                Build.Index=Index;
                AddString(Build);
                Index++;
            }
        }
    }

    Build.Index=Index;
    lstrcpy(Build.SourceCode,"");
    AddString(Build);
    Index++;

    Build.Index=Index;
    lstrcpy(Build.SourceCode,".data?");
    AddString(Build);
    Index++;

    Build.Index=Index;
    lstrcpy(Build.SourceCode,"");
    AddString(Build);
    Index++;

    Build.Index=Index;
    lstrcpy(Build.SourceCode,".code");
    AddString(Build);
    Index++;

    // entry name
    Build.Index=Index;
    lstrcpy(Build.SourceCode,WizardOptions.EntryName);
    lstrcat(Build.SourceCode,":");
    AddString(Build);
    Index++;

    int iLoop,iApiLoop;
    DWORD_PTR Address;
    ItrXref Xitr;
    bool FoundApi=FALSE;
    
    // Build code
    if(CodeSections.size()!=0){
        for(int iSearch=0;iSearch<CodeSections.size();iSearch++){
            // EntryPoint Function, Only 1 !!
            if(CodeSections[iSearch].Start==OEP){
               for(iLoop=0;iLoop<DisasmDataLines.size();iLoop++){
                   FoundApi=FALSE;
				   FoundJump=FALSE;
				   FoundRef=FALSE;
                   Address=StringToDword(DisasmDataLines[iLoop].GetAddress());
                   if(Address>=CodeSections[iSearch].Start && Address<=CodeSections[iSearch].End)
                   {
                       if(Address!=CodeSections[iSearch].Start){
                           Xitr = XrefData.find((DWORD)Address);
                           if(Xitr!=XrefData.end())
                           {
                               Build.Index=Index; 
                               wsprintf(Build.SourceCode,"ref_%08X:",Address);
                               AddString(Build);
                               Index++;
                           }
                       }
                       
                       Build.Index=Index; 
					   // Parse the String References with: instruction Offset <name_of_ref>
					   try{
						   DataItr=DataSection.begin();
						   for(;DataItr!=DataSection.end();DataItr++){
							   int Data=(*DataItr).first;
							   if(Data==iLoop){
								   FoundRef=TRUE;
								   lstrcpy(Temp2,DisasmDataLines[iLoop].GetMnemonic());
								   ptr=Temp2;
								   while(*ptr!=' '){
									   ptr++;
								   }
								   *ptr=NULL;
								   
								   if(lstrcmp(Temp2,"PUSH")==0 || lstrcmp(Temp2,"push")==0){
									   wsprintf(Build.SourceCode,"%s OFFSET %s",Temp2,(*DataItr).second);
								   }
								   else{
									   if(lstrcmp(Temp2,"MOV")==0 || lstrcmp(Temp2,"mov")==0 ||
										   lstrcmp(Temp2,"LEA")==0 || lstrcmp(Temp2,"lea")==0
										   )
									   {
										   lstrcpy(Temp2,DisasmDataLines[iLoop].GetMnemonic());
										   ptr=Temp2;
										   while(*ptr!=','){
											   ptr++;
										   }
										   ptr++;
										   *ptr=NULL;
										   wsprintf(Build.SourceCode,"%s OFFSET %s",Temp2,(*DataItr).second);
									   }
								   }
								   
								   AddString(Build);
								   Index++;
								   ProcFrame++;
								   break;
							   }
						   }
					   }
					   catch(...){}
					   
					   if(FoundRef==TRUE){ // process next isntruction
						   continue;
					   }

					   try{
						   for(iApiLoop=0;iApiLoop<ImportsLines.size();iApiLoop++){
							   if(ImportsLines[iApiLoop] == iLoop){
								   FoundApi=TRUE;
								   lstrcpy(Temp,DisasmDataLines[iLoop].GetMnemonic());
								   ptr=Temp;
								   while(*ptr!='!'){
									   ptr++;
								   }
								   
								   ptr++;
								   lstrcpy(Temp2,ptr); // API Name
								   ptr=Temp;
								   // Get CALL / JMP
								   while(*ptr!=' '){
									   ptr++;
								   }
								   *ptr=NULL;
								   
								   wsprintf(Build.SourceCode,"%s %s",Temp,Temp2);
								   if(WizardOptions.AddComments==TRUE)
								   {
									   wsprintf(Temp," %s",DisasmDataLines[iLoop].GetComments());
									   lstrcat(Build.SourceCode,Temp);
								   }
								   break;
							   }
						   }
					   }
					   catch(...){}
                        
                       if(FoundApi==FALSE){
						   // check for jump here
						   for(int i=0;i<DisasmCodeFlow.size();i++){
							   if(
								   DisasmCodeFlow[i].Current_Index==iLoop &&
								   (DisasmCodeFlow[i].BranchFlow.Jump==TRUE ||
								   DisasmCodeFlow[i].BranchFlow.Call==TRUE)
								 )
							   {
								wsprintf(Temp2,"%s",DisasmDataLines[iLoop].GetMnemonic());
								FixJump(Temp2,DisasmCodeFlow[i].BranchFlow.Jump);
								lstrcpy(Build.SourceCode,Temp2);
								AddString(Build);
							    Index++;
								FoundJump=TRUE;
								break;
							   }
						   }
						   
						   if(FoundJump==TRUE){
							   ProcFrame++;
							   continue;
						   }
						   
						   // all other instructions goes here.
                           lstrcpy(Temp2,DisasmDataLines[iLoop].GetMnemonic());

							// Eliminate/Fix Assembly: RET/LEAVE/PUSH EBP/MOV EBP,ESP 
							if(strstr(Temp2,"RET")!=NULL){
								Temp2[3]=NULL;
							}
							else{
								if(strstr(Temp2,"LEAVE")!=NULL){
									ProcFrame++;
									continue;
								}
							}

						   lstrcpy(Build.SourceCode,Temp2);
						   if(WizardOptions.AddComments==TRUE){  // Add Comments
                               wsprintf(Temp," %s",DisasmDataLines[iLoop].GetComments());
                               lstrcat(Build.SourceCode,Temp);
                           }
						   
                       }

                       AddString(Build);
                       Index++;
					   ProcFrame++;
                   }
               }
            }
            else{
				//
                // Analyze Procedure Body
				//
				// Eliminate "PUSH EBP" / "MOV EBP, ESP"
				// At a beginning of a new proc if found/exists.
				ProcFrame=1; 
                Build.Index=Index;
                lstrcpy(Build.SourceCode,"");
                AddString(Build);
                Index++;

                Build.Index=Index;
                lstrcpy(Build.SourceCode,CodeSections[iSearch].Proc);
                AddString(Build);
                Index++;
                
                for(iLoop=0;iLoop<DisasmDataLines.size();iLoop++){
                    Address=StringToDword(DisasmDataLines[iLoop].GetAddress());
                    FoundApi=FALSE;
					FoundJump=FALSE;
					FoundRef=FALSE;
                    if(Address>=CodeSections[iSearch].Start && Address<=CodeSections[iSearch].End){
                        if(Address!=CodeSections[iSearch].Start){
                            Xitr = XrefData.find((DWORD)Address);
                            if(Xitr!=XrefData.end()){
                                Build.Index=Index; 
                                wsprintf(Build.SourceCode,"ref_%08X:",Address);
                                AddString(Build);
                                Index++;
                            } 
                        }

                        Build.Index=Index;

						// Parse the String References with: instruction Offset <name_of_ref>
						DataItr=DataSection.begin();
						for(;DataItr!=DataSection.end();DataItr++){
							int Data=(*DataItr).first;
							if(Data==iLoop){
								FoundRef=TRUE;
								lstrcpy(Temp2,DisasmDataLines[iLoop].GetMnemonic());
								ptr=Temp2;
								while(*ptr!=' '){
                                    ptr++;
                                }
								*ptr=NULL;

								if(lstrcmp(Temp2,"PUSH")==0 || lstrcmp(Temp2,"push")==0){
									wsprintf(Build.SourceCode,"%s OFFSET %s",Temp2,(*DataItr).second);
								}
								else
								{
									if(lstrcmp(Temp2,"MOV")==0 || lstrcmp(Temp2,"mov")==0 ||
									   lstrcmp(Temp2,"LEA")==0 || lstrcmp(Temp2,"lea")==0
									  )
									{
										lstrcpy(Temp2,DisasmDataLines[iLoop].GetMnemonic());
										ptr=Temp2;
										while(*ptr!=','){
											ptr++;
										}
										ptr++;
										*ptr=NULL;
										wsprintf(Build.SourceCode,"%s OFFSET %s",Temp2,(*DataItr).second);
									}
								}

								AddString(Build);
								Index++;
								ProcFrame++;
								break;
							}
						}

						if(FoundRef==TRUE){ // process next isntruction
							continue;
						}

                        for(iApiLoop=0;iApiLoop<ImportsLines.size();iApiLoop++){
                            if(ImportsLines[iApiLoop] == iLoop){
                                FoundApi=TRUE;
                                lstrcpy(Temp,DisasmDataLines[iLoop].GetMnemonic());
                                ptr=Temp;
                                while(*ptr!='!'){
                                    ptr++;
                                }

                                ptr++;
                                lstrcpy(Temp2,ptr); // API Name
                                ptr=Temp;
                                // Get CALL / JMP
                                while(*ptr!=' '){
                                    ptr++;
                                }
                                *ptr=NULL;
                                wsprintf(Build.SourceCode,"%s %s",Temp,Temp2);
                                break;
                            }
                        }

                        if(FoundApi==FALSE){
						   // Check for jump instruction
							for(int i=0;i<DisasmCodeFlow.size();i++){
								if(
									DisasmCodeFlow[i].Current_Index==iLoop &&
									(DisasmCodeFlow[i].BranchFlow.Jump==TRUE ||
									DisasmCodeFlow[i].BranchFlow.Call==TRUE)
									)
								{
									wsprintf(Temp2,"%s",DisasmDataLines[iLoop].GetMnemonic());
									FixJump(Temp2,DisasmCodeFlow[i].BranchFlow.Jump);
									lstrcpy(Build.SourceCode,Temp2);
									AddString(Build);
									Index++;
									FoundJump=TRUE;
									break;
								}
							}

							// Skip next code when jump fix accourd.
							if(FoundJump==TRUE){
							   ProcFrame++;
							   continue;
							}

							// all other instructions goes here.
							lstrcpy(Temp2,DisasmDataLines[iLoop].GetMnemonic());

							// Eliminate/Fix Assembly: RET/LEAVE/PUSH EBP/MOV EBP,ESP 
							if(strstr(Temp2,"RET")!=NULL){
								Temp2[3]=NULL;
							}
							else{
								if(strstr(Temp2,"LEAVE")!=NULL){ // eliminate LEAVE
									ProcFrame++;
									continue;
								}
								else{
									// eliminate PUSH EBP - proc stack frame
									if(strstr(Temp2,"PUSH EBP")!=NULL && ProcFrame==1){	
										ProcFrame++;
										continue;
									}
									else{
										// eliminate MOV EBP, ESP - proc stack frame
										if(strstr(Temp2,"MOV EBP, ESP")!=NULL && ProcFrame==2){
											ProcFrame++;
											continue;
										}
									}
								}
							}

							lstrcpy(Build.SourceCode,Temp2);
                            if(WizardOptions.AddComments==TRUE){ // Add Comments
                                wsprintf(Temp," %s",DisasmDataLines[iLoop].GetComments());
                                lstrcat(Build.SourceCode,Temp);
                            }
                        }

						// Process other instructions
                        AddString(Build);
                        Index++;
						ProcFrame++;
                    }
                }

                lstrcpy(Temp,CodeSections[iSearch].Proc);
                iLoop=0;
                while(Temp[iLoop]!=' '){
                    iLoop++;
                }
                iLoop++;
                Temp[iLoop]=NULL;
                lstrcat(Temp," endp"); // end of a procedure
                
                Build.Index=Index; 
                lstrcpy(Build.SourceCode,Temp);
                AddString(Build);
                Index++;
            }
        }
    }

    Build.Index=Index; 
    lstrcpy(Build.SourceCode,"");
    AddString(Build);
    Index++;

    Build.Index=Index; 
	// end of entry point
    wsprintf(Build.SourceCode,"end %s",WizardOptions.EntryName);
    AddString(Build);
    Index++;
    ListView_SetItemCountEx(hList,Index,NULL);
}

//////////////////////////////////////////////////////////////////////////
//					FIX Jump Instruction for Source Code				// 
//////////////////////////////////////////////////////////////////////////
//
// The function will fix string "JMP 00401131" -> "JMP ref_00401131"
//
void FixJump(char* Instruction,bool fixation)
{
	DWORD Address,i;
	char Temp[128],Temp2[128];
	bool Found=FALSE;
	char* ptr;

	lstrcpy(Temp2,Instruction);
	ptr=Temp2;
	while(*ptr!=' '){
		ptr++;
	}
	*ptr=NULL;

	if(fixation==TRUE){
		wsprintf(Temp,"%s ref_",Temp2);
	}
	else{
		wsprintf(Temp,"%s proc_",Temp2);
	}

	ptr=Instruction;
	while(*ptr!=' '){
		ptr++;
	}
	ptr++;

	if(fixation==FALSE){ // call instruction
		Address=StringToDword(ptr);
		for(i=0;i<fFunctionInfo.size();i++){
			if(Address==fFunctionInfo[i].FunctionStart)
			{
				Found=TRUE;
				break;
			}
		}

		if(Found==TRUE){
			if(lstrcmp(fFunctionInfo[i].FunctionName,"")!=0){
				wsprintf(Temp,"%s %s",Temp2,fFunctionInfo[i].FunctionName);
			}
			else{
				wsprintf(Temp,"%s proc_%08X",Temp2,Address);
			}
		}
		else{
			lstrcat(Temp,ptr);
		}
	}
	else lstrcat(Temp,ptr);
	lstrcpy(Instruction,Temp);
}


//////////////////////////////////////////////////////////////////////////
//				Build ASM From Disassembly Window						//
//////////////////////////////////////////////////////////////////////////
void BuildSourceCode()
{
    HWND hPrev = GetDlgItem(PrevhWnd,IDC_WIZARD_PREVIEW);
    HANDLE hFile;
    DWORD_PTR Item;
	DWORD WritenBytes;
    LVITEM LvItem;
    char Text[256];
    
    // Initialize the List view ITEM struct
    memset(&LvItem,0,sizeof(LvItem));
    LvItem.mask=LVIF_TEXT;
    LvItem.pszText=Text;
    LvItem.cchTextMax=256;

    hFile = CreateFile(prjFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hFile==INVALID_HANDLE_VALUE){    
		MessageBox(PrevhWnd,"Can't Create Source File!","Error",MB_OK);
        return;
    }
    Item=-1;

    while((Item=SendMessage(hPrev,LVM_GETNEXTITEM,Item,LVNI_ALL))!=-1){
        SendMessage(hPrev,LVM_GETITEMTEXT, Item, (LPARAM)&LvItem);
        wsprintf(Text,"%s\r\n",LvItem.pszText);
        WriteFile(hFile,Text,StringLen(Text),&WritenBytes,NULL);
    }

    MessageBox(PrevhWnd,"Source Created!","Notice",MB_OK);
    CloseHandle(hFile);
    TerminateThread(hDisasmThread,0);
    ExitThread(0);
}


void AddType(HWND hWnd)
{
    char* Buffer;
    char* Str;
    char Buf[256],Var[128];
    DWORD_PTR len,index;
    
    index =  SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_GETCURSEL ,0,0);
    index=SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_GETITEMDATA ,index,0);
    
	if(index==-1){ // no params
        return;
	}

    if(index==7){ // PROTO
        GetWindowText(GetDlgItem(hWnd,IDC_PARAM_OTHER),Buf,256);
        len=StringLen(Buf);
        Buffer = new char [len+1];
        lstrcpy(Buffer,Buf);
    }
    else{
        len=SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_GETLBTEXTLEN,(WPARAM)index+1,0);
        Buffer = new char [len+1];
        SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_GETLBTEXT,(WPARAM)index+1,(LPARAM)Buffer);
    }
    GetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),Buf,256);
    if(StringLen(Buf)==0){
        delete[]Buffer;
        return;
    }

    Str = new char[StringLen(Buffer) + StringLen(Buf) + 4];
    if(FirstParam==TRUE){
        wsprintf(Str,"%s :%s",Buf,Buffer);
    }
    else{
        wsprintf(Str,"%s,:%s",Buf,Buffer);
    }

    SetWindowText(GetDlgItem(hWnd,IDC_PROTO_EDIT),Str);
    delete[]Str;
    delete[]Buffer;
    
    if(index==7){ //PROC
        GetWindowText(GetDlgItem(hWnd,IDC_PARAM_OTHER),Buf,256);
        len=StringLen(Buf);
        Buffer = new char [len+1];
        lstrcpy(Buffer,Buf);
    }
    else{
        Buffer = new char [len+1];
        SendMessage(GetDlgItem(hWnd,IDC_PARAM_TYPE),CB_GETLBTEXT,(WPARAM)index+1,(LPARAM)Buffer);
    }
    GetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),Buf,256);
    if(StringLen(Buf)==0){
        delete[]Buffer;
        return;
    }
    wsprintf(Var,"Var%d",VarCount);
    Str = new char[StringLen(Buffer) + StringLen(Buf) + StringLen(Var) + 5];
    
    if(FirstParam==TRUE){
        wsprintf(Str,"%s %s:%s",Buf,Var,Buffer);
    }
    else{
        wsprintf(Str,"%s,%s:%s",Buf,Var,Buffer);
    }
    
    SetWindowText(GetDlgItem(hWnd,IDC_PROC_EDIT),Str);
    delete[]Str;
    delete[]Buffer;
    FirstParam=FALSE;
    VarCount++;
}