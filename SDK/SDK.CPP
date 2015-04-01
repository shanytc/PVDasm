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


 File: SDK.cpp (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#include "SDK.H"

//////////////////////////////////////////////////////////////////////////
//						GLOBAL VARIABLES								//
//////////////////////////////////////////////////////////////////////////

DllArray hDllArray;

//////////////////////////////////////////////////////////////////////////
//						EXTERNAL VARIABLES								//
//////////////////////////////////////////////////////////////////////////

extern HWND			Main_hWnd;
extern char			*OrignalData;
extern bool			DisassemblerReady,LoadedPe,LoadedNonePe;
extern DWORD_PTR	hFileSize;

//////////////////////////////////////////////////////////////////////////
//							FUNCTIONS									//
//////////////////////////////////////////////////////////////////////////

//
// This Function check for the Message Located in wParam
// If it is equal to a Plugin Message, it will execute the Plugin.
//
void CheckForPluginPress(WPARAM wParam)
{
    DWORD MLoop=0;
    for(;MLoop<hDllArray.size();MLoop++)
    {
        if(LOWORD(wParam) == WM_PLUGIN+MLoop)
        {
            ExecutePlugin(MLoop);
            break;
        }
    }
}


//
// This Function will search/enumerate for all plugins
// Located at \Plugins\*.dll Dir.
// and will Load the DLL into pvdasm process.
//
void EnumPlugins()
{
  WIN32_FIND_DATA W32FindData; // Find Data Struct
  HANDLE hFile;
  HMODULE hDll;
  DWORD dwRetVal=0;
  char cFilePath[MAX_PATH]="";
  char cWildCard[]="*.dll";
  
  ZeroMemory(&W32FindData,0); // Clear the struct
  
  // Get Current Path
  GetModuleFileName(NULL,cFilePath,MAX_PATH-1);
  // Remove the 'exe' Extension, and return dir path.
  ExtractFilePath(cFilePath);
  // Attach "\Plugins"
  lstrcat(cFilePath,"Plugins");
  // Set Current path for the Plugin's dir.
  SetCurrentDirectory(cFilePath);

  // Search for the first file in the plugins dir.
  hFile=FindFirstFile(cWildCard,&W32FindData);

  // Check if file is found
  if(hFile!=INVALID_HANDLE_VALUE)
  {
	// Load the DLL
    hDll = LoadLibrary(W32FindData.cFileName);

	// Store the DLL handle in a linked list.
    if(hDll != NULL){
        hDllArray.insert(hDllArray.end(),hDll);
    }

	// Search for all DLLs found, load them, and store thier handles.
    while(FindNextFile(hFile,&W32FindData))
    {
       hDll = LoadLibrary(W32FindData.cFileName);
       
       if(hDll != NULL){
          hDllArray.insert(hDllArray.end(),hDll);
       }
    }
  }
}


//
// This fucntion Creates the menu tree 'Plugins'
// And attaches the plugin's name(s), as well data item.
//
void LoadPlugins(HWND hWnd)
{
    HMENU hMenu = GetMenu(hWnd);
    HMENU hNewMenu;
    int loopy=0;
    char PluginName[128]="";
    
    if(hDllArray.size()==0)
        return;

    // Create Dynamic Popup Menu,
    // Accourding to the number of Plugins Loaded.
    hNewMenu=CreatePopupMenu();
    AppendMenu(hMenu,MF_POPUP,(INT)hNewMenu,"&Plugins");

	// Set/Create Menu Items with Plugin's Name
    for(int i=0;i<hDllArray.size();i++)
    {
        // Function returns the Caption of the plugin.
        PLG_INFO pPlgInfo = (PLG_INFO)GetProcAddress((HINSTANCE)hDllArray[i],"PluginInfo");
        // Check if this is a valid pvdasm plugin.
		if(pPlgInfo!=NULL)
        {
            wsprintf(PluginName,"%ld - %s",loopy+1,pPlgInfo());
            AppendMenu(hNewMenu,MF_STRING,i+WM_PLUGIN,PluginName);
            loopy++;
        }
    }

	// Redraw Menu
    DrawMenuBar(Main_hWnd);
}


//
// Function executes the Plugin selected in menu.
//
void ExecutePlugin(DWORD dwIndex)
{
    if(dwIndex<0 || dwIndex>=hDllArray.size())
        return;

    // Get Ptr to the Install Function
    INSTALL pFunc = (INSTALL)GetProcAddress((HINSTANCE)hDllArray[dwIndex],"InstallDll");
	
	// check for valid plugin function
    if(pFunc!=NULL)
    {   
		// Load plugin only if a file is loaded
        if(DisassemblerReady==TRUE || LoadedPe==TRUE || LoadedNonePe==TRUE)
        {
            PLUGINFO Plg;

            // Prepare Basic Information to send to the plugin
            Plg.DisasmReady = DisassemblerReady;
            Plg.LoadedPe = LoadedPe;
            Plg.Parent_hWnd = &Main_hWnd;
            Plg.FilePtr = (BYTE*)OrignalData;
            Plg.FileSize = (DWORD)hFileSize;

            // Call Plugin & Send Information
            pFunc(Plg);
        }
        else
            MessageBox(Main_hWnd,"Can't Load Plugin, No File is Loaded!","Can't Execute Plugin",MB_OK|MB_ICONEXCLAMATION);
    }
}

//
// This fucntion Free the plugin's handlers 
// When pvdasm is shuting down.
//
void FreePlugins()
{
	// Scan all handlers
    for(int i=0;i<hDllArray.size();i++)
    {
		// Free if valid
        if(hDllArray[i]!=NULL)
            FreeLibrary(hDllArray[i]);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////