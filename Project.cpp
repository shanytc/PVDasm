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

 File: Project.cpp (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#include "Project.h"

using namespace std; // Use Microsoft STL Templates

//======================================================================================
//=================================  TYPE DEFINES ======================================
//======================================================================================

typedef vector<CDisasmData>				DisasmDataArray;
typedef vector<CODE_BRANCH>				CodeBranch;
typedef vector<FUNCTION_INFORMATION>	FunctionInfo;
typedef vector<BRANCH_DATA>				BranchData;  
typedef vector<int>						DisasmImportsArray,IntArray;
typedef multimap<const int, int>		XRef,MapTree;
typedef pair<const int const, int>		XrefAdd,MapTreeAdd;
typedef XRef::iterator					ItrXref;
typedef MapTree::iterator				TreeMapItr;
//======================================================================================
//============================ EXTERNAL VARIABLES ======================================
//======================================================================================

extern DisasmDataArray		DisasmDataLines;  // disassembled informationj
extern CDisasmColor			DisasmColors;     // colors for the disassembly outoput
extern CodeBranch			DisasmCodeFlow;   // codeflow of the data
extern BranchData			BranchTrace;      // saved branching information
extern DisasmImportsArray	ImportsLines;     // saved imports indexes in disassembly
extern DisasmImportsArray	StrRefLines;      // saved string reference idnexes in disassembly
extern XRef					XrefData;         // xreference information
extern DWORD_PTR			hFileSize;
extern HWND					Main_hWnd;
extern HWND					hWndTB;
extern bool					FilesInMemory,DisassemblerReady,LoadedPe,LoadedPe64,LoadedNe;
extern char					szFileName[MAX_PATH];
extern HANDLE				hFile,hFileMap;
extern char					*OrignalData;
extern IMAGE_DOS_HEADER		*doshdr;
extern IMAGE_NT_HEADERS		*nt_hdr;
extern IMAGE_SECTION_HEADER	*SectionHeader;
extern MapTree				DataAddersses; // EntryPoints & Data In Code information
extern FunctionInfo			fFunctionInfo;

//////////////////////////////////////////////////////////////////////////
//                        DEFINES                                       //
//////////////////////////////////////////////////////////////////////////

#define AddNew(key,data) XrefData.insert(XrefAdd(key,data));
#define AddNewData(key,data) DataAddersses.insert(MapTreeAdd(key,data));
#define AddFunction(Function) fFunctionInfo.insert(fFunctionInfo.end(),Function)

#define USE_MMX

#ifdef USE_MMX
	#define StringLen(str) xlstrlen(str) // new lstrlen with mmx function
#else
    #define StringLen(str) lstrlen(str) // old c library strlen
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void SaveProject()
{    
    HANDLE _hFile;   
    SAVE_DATA Save;
    CODE_BRANCH CBranch;
    BRANCH_DATA BData;
	DWORD_PTR i=0,j=0;
	DWORD WritenBytes,Size=0;
	TreeMapItr itr;
    char Path[MAX_PATH]="",ProjectName[MAX_PATH]="",ExeName[MAX_PATH]="";


    strcpy_s(ProjectName,StringLen(szFileName)+1,szFileName);
    GetExeName(ProjectName);
    while(ProjectName[i]!='.')
        i++;
    
    ProjectName[i]='\0';
    lstrcpy(ExeName,ProjectName);

    GetModuleFileName(NULL,Path,MAX_PATH-1);
    ExtractFilePath(Path);
    lstrcat(Path,"Projects\\");
    if(SetCurrentDirectory(Path)==NULL)
	{
		MessageBox(Main_hWnd,"Can't Find Projects Directory!\nSaving to default Dir: c:\\","Notice",MB_OK);
		SetCurrentDirectory("c:\\");
	}

    // Create the Project File
    lstrcat(ProjectName,".pvprj");
	wsprintf(Path,"Creating project: %s",ProjectName);
	OutDebug(Main_hWnd,Path);
    _hFile=CreateFile(ProjectName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFile);
        return;
    }

    // ============================
    //  Write Project File Names
    // ============================
    wsprintf(Path,"%s_DisasmData.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);
    
    wsprintf(Path,"%s_ImportsData.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);

    wsprintf(Path,"%s_CodeFlowData.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);

    wsprintf(Path,"%s_BranchTrace.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);

    wsprintf(Path,"%s_XRefData.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);

    wsprintf(Path,"%s_StrRefData.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);
    
    wsprintf(Path,"%s_DataInCode.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);

    wsprintf(Path,"%s_FuncEntrypoint.pvd\r\n",ExeName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);

    wsprintf(Path,"%s\r\n",szFileName);
    WriteFile(_hFile,Path,StringLen(Path),&WritenBytes,NULL);

    // Close File Handler
    CloseHandle(_hFile);

    // =============================
    // Write Disassembly Data File
    // =============================
    wsprintf(Path,"%s_DisasmData.pvd",ExeName);
    
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;
    
    j=DisasmDataLines.size();

    try{
        for(i=0;i<j;i++)
        {
            memset(&Save,0,sizeof(Save)); // Intialize the Struct
            // Get Disassembly Information
            lstrcpy(Save.Address,DisasmDataLines[i].GetAddress());                
            lstrcpy(Save.Opcode,DisasmDataLines[i].GetCode());        
            lstrcpy(Save.Assembly,DisasmDataLines[i].GetMnemonic());         
            lstrcpy(Save.Remarks,DisasmDataLines[i].GetComments());
			lstrcpy(Save.Reference,DisasmDataLines[i].GetReference());
            Save.SizeOfInstruction=DisasmDataLines[i].GetSize();
            Save.SizeOfPrefix = DisasmDataLines[i].GetPrefixSize();
            // Store the Data in the file
            WriteFile(_hFile,&Save,sizeof(Save),&WritenBytes,NULL);
        }
    }
    catch(...)
    {
        MessageBox(Main_hWnd,"Error In Saving project!","Save Error",MB_OK|MB_ICONEXCLAMATION);
        CloseHandle(_hFile);        
        return;
    }

    CloseHandle(_hFile);

    // ==========================
    // Write Improrts Data File
    // ==========================
    wsprintf(Path,"%s_ImportsData.pvd",ExeName);
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;

    j = ImportsLines.size();
        
    for(i=0;i<j;i++)
    {
        Size = ImportsLines[i];
        WriteFile(_hFile,&Size,sizeof(int),&WritenBytes,NULL);
    }

    CloseHandle(_hFile);

    // ==========================
    // Write Code Flow Data File
    // ==========================
    wsprintf(Path,"%s_CodeFlowData.pvd",ExeName);
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;

    j = DisasmCodeFlow.size();

    for(i=0;i<j;i++)
    {
        memset(&CBranch,0,sizeof(CODE_BRANCH)); // Initalize
        // Move Data to struct
        CBranch.Branch_Destination = DisasmCodeFlow[i].Branch_Destination;
        CBranch.BranchFlow.Call = DisasmCodeFlow[i].BranchFlow.Call;
        CBranch.BranchFlow.Jump = DisasmCodeFlow[i].BranchFlow.Jump;
        CBranch.Current_Index = DisasmCodeFlow[i].Current_Index;
        // Store struct in file
        WriteFile(_hFile,&CBranch,sizeof(CODE_BRANCH),&WritenBytes,NULL);
    }

    CloseHandle(_hFile);

    //===============================
    // Write Branch Trace Data File
    //===============================
    wsprintf(Path,"%s_BranchTrace.pvd",ExeName);
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;

    j = BranchTrace.size();

    for(i=0;i<j;i++)
    {
        memset(&BData,0,sizeof(BRANCH_DATA));
        BData.Call = BranchTrace[i].Call;
        BData.ItemIndex = BranchTrace[i].ItemIndex;
        BData.Jump = BranchTrace[i].Jump;
        WriteFile(_hFile,&BData,sizeof(BRANCH_DATA),&WritenBytes,NULL);
    }

    CloseHandle(_hFile);

    // =============================
    // Write Xreferences Data File
    // =============================
    wsprintf(Path,"%s_XRefData.pvd",ExeName);
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;
    
    ItrXref Itr = XrefData.begin();
    
    for(;Itr!=XrefData.end();Itr++)
    {
        Size = (*Itr).first;
        WriteFile(_hFile,&Size,sizeof(DWORD),&WritenBytes,NULL);
        Size = (*Itr).second;
        WriteFile(_hFile,&Size,sizeof(DWORD),&WritenBytes,NULL);
    }

    CloseHandle(_hFile);

    // ===================================
    // Write String References Data File
    // ===================================
    wsprintf(Path,"%s_StrRefData.pvd",ExeName);
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;

    j = StrRefLines.size();

    for(i=0;i<j;i++)
    {
        Size = StrRefLines[i];
        WriteFile(_hFile,&Size,sizeof(int),&WritenBytes,NULL);
    }

	CloseHandle(_hFile);

	// ==============================
	// Write Data In Code Index File
	// ==============================
	wsprintf(Path,"%s_DataInCode.pvd",ExeName);
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;    
    
	itr = DataAddersses.begin();
	for(;itr!=DataAddersses.end();itr++)
	{
		j=(*itr).first;
		WriteFile(_hFile,&j,sizeof(int),&WritenBytes,NULL);
		j=(*itr).second;
		WriteFile(_hFile,&j,sizeof(int),&WritenBytes,NULL);
	}

    CloseHandle(_hFile);

	// ====================================
	// Write Function EntryPoint Index File
	// ====================================
	wsprintf(Path,"%s_FuncEntrypoint.pvd",ExeName);
    _hFile=CreateFile(Path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
        return;

	for(i=0;i<fFunctionInfo.size();i++)
	{
		j=fFunctionInfo[i].FunctionStart;
		WriteFile(_hFile,&j,sizeof(int),&WritenBytes,NULL);
		j=fFunctionInfo[i].FunctionEnd;
		WriteFile(_hFile,&j,sizeof(int),&WritenBytes,NULL);
		j=StringLen(fFunctionInfo[i].FunctionName); // length of function name
		WriteFile(_hFile,&j,sizeof(int),&WritenBytes,NULL);
		WriteFile(_hFile,&fFunctionInfo[i].FunctionName,StringLen(fFunctionInfo[i].FunctionName),&WritenBytes,NULL); // write function name
	}

    CloseHandle(_hFile);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void LoadProject()
{
    HANDLE _hFile;
    LOAD_DATA Load;
	FUNCTION_INFORMATION fFunc;
    CDisasmData DisasmData("","","","",0,0,"");// empty members in object
    OPENFILENAME ofn;
    CODE_BRANCH CBranch;
    BRANCH_DATA BData;
    DWORD WritenBytes=0;
	DWORD_PTR Index=0;
	DWORD Key,Data;
    int i=0,j=0,RetVal;
    char FileName[MAX_PATH]="";
    char Path[MAX_PATH]="";
    char Files[MAX_PATH]="";
    char FileNames[MAX_PROJECT_FILES][MAX_PATH]={"","","","","","",""}; // Intialize

    // Inialuize File Browser Struct
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of Struct
    ofn.hwndOwner = Main_hWnd;
    ofn.lpstrFilter = "PVDasm Project (*.pvprj)\0*.pvprj\0"; // File Filter
    ofn.lpstrFile=FileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt="prj";

    // load File Browser Dialog
    if(GetOpenFileName(&ofn)!=NULL)
    {
        // Open Selected Line
        _hFile = CreateFile(FileName,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(_hFile==INVALID_HANDLE_VALUE){
            return;
		}

        // Read MAX_PROJECT_FILES = 6 Lines
        for(i=0;i<MAX_PROJECT_FILES;i++)
        {
            // Read Line i
            do{
                RetVal=ReadFile(_hFile,&Files[j],sizeof(char),&WritenBytes,NULL);
                
                // Did We Read beyond the File's End?
                if(WritenBytes==NULL && RetVal!=0)
                    return;

                j++;
            }
            while(Files[j-1]!=0x0A); // Read a line

            Files[j-2]='\0'; // Remove '\r\n'
            j=0;

            wsprintf(FileNames[i],"%s",Files); // Copy Line to Buffer
        }

        CloseHandle(_hFile);

    }else return;

	OutDebug(Main_hWnd,"Loading selected project...");
    // Get Current Module Path
    GetModuleFileName(NULL,Path,MAX_PATH-1);
    // Remove the Exe Name
    ExtractFilePath(Path);
    // Add Project Dir
    lstrcat(Path,"Projects\\");
    // Move to the Projects Directory
    SetCurrentDirectory(Path);

    // Check if a Disassemble Project is already Opened
    // And release the information if it is.
    if(FilesInMemory==TRUE)
    {   
        // Unmaping File
        if(UnmapViewOfFile(OrignalData)==0) // סגירת מיפוי הקובץ מהזיכרון        	
            return;
        // Close the map handle
        if(CloseHandle(hFileMap)==NULL) // סגירת המזהה של המפה        	
            return;
        // Close the file handle
        if(CloseHandle(hFile)==NULL) // סגירת המזהה של הקובץ        		
            return;
        
		// clear the content of the information,
		// of the last disassembled file.
        FreeDisasmData();
    }
    
    // ===================
    //  Load Disasm Data
    // ===================
    _hFile = CreateFile(FileNames[0],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;
    }

    // Load Disassembly Data
    while(TRUE)
    {
        RetVal=ReadFile(_hFile,&Load,sizeof(LOAD_DATA),&WritenBytes,NULL);
        // Check End Of File
        if(RetVal && !WritenBytes)
        {
            break;
        }

        // Insert New disassembly class object in Container List
        DisasmDataLines.insert(DisasmDataLines.end(),DisasmData);
        assert(DisasmDataLines.size() == Index+1 && DisasmDataLines.capacity() >= Index+1);
        // Inert Data into Container's Index
        DisasmDataLines[Index].SetAddress(Load.Address);
        DisasmDataLines[Index].SetCode(Load.Opcode);
        DisasmDataLines[Index].SetMnemonic(Load.Assembly);
        DisasmDataLines[Index].SetComments(Load.Remarks);
		DisasmDataLines[Index].SetReference(Load.Reference);
        DisasmDataLines[Index].SetSize(Load.SizeOfInstruction);        
        Index++;
    }

    CloseHandle(_hFile);

    // ==========================
    //    Load Improrts Data 
    // ==========================    
    _hFile = CreateFile(FileNames[1],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    // Check End Of File
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;
    }
    
    while(TRUE)
    {
      RetVal=ReadFile(_hFile,&Index,sizeof(int),&WritenBytes,NULL);

      if(RetVal && !WritenBytes)
      {
          break;
      }

      ImportsLines.insert(ImportsLines.end(),(DWORD)Index);      
    }

    CloseHandle(_hFile);
    
    // ==========================
    //    Load Code Flow Data  
    // ==========================
    
    _hFile = CreateFile(FileNames[2],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    // Check End Of File
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;  
    }
    
    while(TRUE)
    {   
      memset(&CBranch,0,sizeof(CODE_BRANCH));  
      RetVal=ReadFile(_hFile,&CBranch,sizeof(CODE_BRANCH),&WritenBytes,NULL);
      
      if(RetVal && !WritenBytes)
      {
          break;
      }
      
      DisasmCodeFlow.insert(DisasmCodeFlow.end(),CBranch);      
    }
    
    CloseHandle(_hFile);

    // ==========================
    //    Load Branch Trace Data
    // ==========================
    _hFile = CreateFile(FileNames[3],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    // Check End Of File
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();        
        return;  
    }
    
    while(TRUE)
    {
      memset(&BData,0,sizeof(BRANCH_DATA));  
      RetVal=ReadFile(_hFile,&BData,sizeof(BRANCH_DATA),&WritenBytes,NULL);

      if(RetVal && !WritenBytes)
      {
          break;
      }

      BranchTrace.insert(BranchTrace.end(),BData);
    }
    
    CloseHandle(_hFile);

    // =======================
    //  Load XReferences Data  
    // =======================

    _hFile = CreateFile(FileNames[4],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    // Check End Of File
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;  
    }
    
    while(TRUE)
    {
      RetVal=ReadFile(_hFile,&Key,sizeof(int),&WritenBytes,NULL);

      if(RetVal && !WritenBytes)
      {
          break;
      }

      RetVal=ReadFile(_hFile,&Data,sizeof(int),&WritenBytes,NULL);
      if(RetVal && !WritenBytes)
          break;

      AddNew(Key,Data) // Add key&data to xref container
    }
    
    CloseHandle(_hFile);

    //=============================
    // Load String References Data 
    //=============================    
    _hFile = CreateFile(FileNames[5],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    // Check End Of File
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;
    }

    while(TRUE)
    {    
      RetVal=ReadFile(_hFile,&Index,sizeof(int),&WritenBytes,NULL);
      // Check End of File
      if(RetVal && !WritenBytes)
      {
          break;
      }
      
      StrRefLines.insert(StrRefLines.end(),(DWORD)Index);
    }

    CloseHandle(_hFile);

	//=========================
	// Load Data Entrypoints
	// ======================

    _hFile = CreateFile(FileNames[6],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    // Check End Of File
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;
    }

    while(TRUE)
    {
      RetVal=ReadFile(_hFile,&Key,sizeof(int),&WritenBytes,NULL);
      
      if(RetVal && !WritenBytes){
          break;
      }

      RetVal=ReadFile(_hFile,&Data,sizeof(int),&WritenBytes,NULL);
      if(RetVal && !WritenBytes){
          break;
	  }

      AddNewData(Key,Data) // Add key&data to xref container
    }

    CloseHandle(_hFile);

	//===========================
	// Load Functions Entrypoints
	// ==========================

    _hFile = CreateFile(FileNames[7],GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    // Check End Of File
    if(_hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;  
    }
    
    while(TRUE)
    {
      RetVal=ReadFile(_hFile,&Key,sizeof(int),&WritenBytes,NULL);
      
      if(RetVal && !WritenBytes){
          break;
      }
      
      RetVal=ReadFile(_hFile,&Data,sizeof(int),&WritenBytes,NULL);
      if(RetVal && !WritenBytes){
          break;
	  }

	  fFunc.FunctionStart=Key;
	  fFunc.FunctionEnd=Data;

	  RetVal=ReadFile(_hFile,&Data,sizeof(int),&WritenBytes,NULL);
      if(RetVal && !WritenBytes){
          break;
	  }

	  if(Data==0)
		RetVal=ReadFile(_hFile,&fFunc.FunctionName,Data,&WritenBytes,NULL);
	  else
	  {
		  RetVal=ReadFile(_hFile,&fFunc.FunctionName,Data,&WritenBytes,NULL);
		  if(RetVal && !WritenBytes){
			  break;
		  }
	  }
	  
	  fFunc.FunctionName[Data]=NULL;
	  AddFunction(fFunc);
    }

    CloseHandle(_hFile);

	//////////////////////////////////////////////////////////////////////////
	// Create the Disassembly enviorment
	// Fix all variables and load the Executable for
	// later disassembly.
	//////////////////////////////////////////////////////////////////////////

    hFile=CreateFile(FileNames[8],
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL );

    if(hFile==INVALID_HANDLE_VALUE)
    {
        FreeDisasmData();
        return;
    }
    
    hFileSize=GetFileSize(hFile,NULL);
    
    if(hFileSize==0xFFFFFFFF)
    {
        FreeDisasmData();
        return;
    }
    
    hFileMap=CreateFileMapping(hFile,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        NULL);

    if(hFileMap==NULL)
    {
        FreeDisasmData();
        return;
    }

    OrignalData=(char*)MapViewOfFile(hFileMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0);

    // Set PE/DOS Heders
    doshdr=(IMAGE_DOS_HEADER*)OrignalData;
	nt_hdr=(IMAGE_NT_HEADERS*)(doshdr->e_lfanew+OrignalData);
    SectionHeader=(IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_hdr->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));

    FilesInMemory=TRUE;
    DisassemblerReady=TRUE;
    LoadedPe=TRUE;


    ShowWindow(GetDlgItem(Main_hWnd,IDC_DISASM),SW_SHOW);
    ListView_SetItemCountEx(GetDlgItem(Main_hWnd,IDC_DISASM),DisasmDataLines.size(),NULL);

	// Activate/enable Menu items
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_EP_SHOW,		(LPARAM) TRUE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_CODE_SHOW,	(LPARAM) TRUE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_ADDR_SHOW,	(LPARAM) TRUE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_SEARCH,		(LPARAM) TRUE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_PATCH,		(LPARAM) TRUE );
	SendMessage ( hWndTB, TB_ENABLEBUTTON, IDC_SAVE_DISASM,	(LPARAM) TRUE );

    HMENU hMenu = GetMenu(Main_hWnd);
    EnableMenuItem ( hMenu, IDC_START_DISASM,		MF_ENABLED );
    EnableMenuItem ( hMenu, IDC_GOTO_START,			MF_ENABLED );
	EnableMenuItem ( hMenu, IDC_GOTO_ENTRYPOINT,	MF_ENABLED );
    EnableMenuItem ( hMenu, IDC_GOTO_ADDRESS,		MF_ENABLED );
    EnableMenuItem ( hMenu, IDM_FIND,				MF_ENABLED );
    EnableMenuItem ( hMenu, IDM_PATCHER,			MF_ENABLED );
	EnableMenuItem ( hMenu, ID_PE_EDIT,				MF_ENABLED );
	EnableMenuItem ( hMenu, IDM_COPY_DISASM_FILE,				MF_ENABLED );
	EnableMenuItem ( hMenu, IDM_COPY_DISASM_CLIP,				MF_ENABLED );
	EnableMenuItem ( hMenu, IDM_SELECT_ALL_ITEMS,				MF_ENABLED );

    if(StrRefLines.size() > 0)
    {
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SHOW_XREF, (LPARAM)TRUE);
        EnableMenuItem(hMenu,IDC_STR_REFERENCES,MF_ENABLED);
    }

    if(ImportsLines.size() > 0)
    {
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SHOW_IMPORTS, (LPARAM)TRUE);
        EnableMenuItem(hMenu,IDC_DISASM_IMP,MF_ENABLED);
    }

    // Select the EntryPoint
    wsprintf(Files,"%08X",nt_hdr->OptionalHeader.AddressOfEntryPoint+nt_hdr->OptionalHeader.ImageBase);
    
    Index=SearchItemText(GetDlgItem(Main_hWnd,IDC_DISASM),Files);
    if(Index!=-1) 
    {
        SelectItem(GetDlgItem(Main_hWnd,IDC_DISASM),Index-1); // select and scroll to item
        SetFocus(GetDlgItem(Main_hWnd,IDC_DISASM));
    }

    GetWindowText(Main_hWnd,Files,256);
    lstrcpy(szFileName,FileName);
    GetExeName(szFileName);
    wsprintf(Path,"%s - %s",Files,szFileName);
    SetWindowText(Main_hWnd,Path);
	wsprintf(Path,"Project \"%s\" Loaded.",szFileName);
	OutDebug(Main_hWnd,Path);
	lstrcpy(szFileName,FileNames[8]);
    IntializeDisasm(LoadedPe,LoadedPe64,LoadedNe,Main_hWnd,OrignalData,szFileName);
    SelectItem(GetDlgItem(Main_hWnd,IDC_DISASM),0);
    UpdateWindow(Main_hWnd);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void FreeDisasmData()
{
    //////////////////////////////////////////////////////////////////////////
    //				Free Disassembler Information From Memory				//
    //////////////////////////////////////////////////////////////////////////

    // Free Disasm data from memory
    DisasmDataLines.clear();
    DisasmDataArray(DisasmDataLines).swap(DisasmDataLines);

    // Free Imports data from memory
    ImportsLines.clear();
    DisasmImportsArray(ImportsLines).swap(ImportsLines);

    // Free String ref data from memory
    StrRefLines.clear();
    DisasmImportsArray(StrRefLines).swap(StrRefLines);

    // Free The Data
    DisasmCodeFlow.clear();
    CodeBranch(DisasmCodeFlow).swap(DisasmCodeFlow);

    // Clear Tracing List
    BranchTrace.clear();
    BranchData(BranchTrace).swap(BranchTrace);

    // Clear Xref List
    XrefData.clear();
    XrefData.swap(XRef(XrefData));

	// Clear content
	DataAddersses.clear();
	MapTree(DataAddersses).swap(DataAddersses);

	// clear content
	fFunctionInfo.clear();
	FunctionInfo(fFunctionInfo).swap(fFunctionInfo);

    FilesInMemory=FALSE;
}