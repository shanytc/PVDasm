 
/*
     8888888b.                  888     888 d8b                        
     888   Y88b                 888     888 Y8P                        
     888    888                 888     888                            
     888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888 
     8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888 
     888        888     888  888  Y88o88P   888 88888888 888  888  888 
     888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P 
     888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"  


     This code is a part of the Proview Plugin SDK.
     Coded by Bengaly 2003-2004
*/

//////////////////////////////////////////////////////////////////////////
//                         INCLUDES                                     //
//////////////////////////////////////////////////////////////////////////
#include "Main.h"

//////////////////////////////////////////////////////////////////////////
//                    GLOBAL VARIABLES                                  //
//////////////////////////////////////////////////////////////////////////

HINSTANCE hDllInstance; // dll instance handler
PLUGINFO  PlgData;      // Global, add 'const' to avoid ovveride
WNDPROC   OldWndProc;   // Used for SubClassing Controls
HWND      Main_hWnd;    // Main Plugin's Dialog HWND

//////////////////////////////////////////////////////////////////////////
////////////////////// Do Not Alter Below Code Section ///////////////////
//////////////////////////////////////////////////////////////////////////

int WINAPI DllMain(HINSTANCE hInstance,DWORD  reason, LPVOID Reserved)
{
	// Store the instance we get
	hDllInstance = hInstance;

	// Return true, we are ready for hook on the system
	return TRUE;
}

// Plugin Instalation
void InstallDll(PLUGINFO plg)
{
    PlgData.DisasmReady = plg.DisasmReady; // Access BOOL  with: PlgData.DisasmReady
    PlgData.FilePtr     = plg.FilePtr;     // Access BYTE* with: Address: PlgData.FilePtr / Data: *PlgData.FilePtr
    PlgData.LoadedPe    = plg.LoadedPe;    // Access BOOL  with: PlgData.LoadedPe 
    PlgData.Parent_hWnd = plg.Parent_hWnd; // Access HWND  with: *PlgData.Parent_hWnd
    PlgData.FileSize    = plg.FileSize;    // Access DWORD with: PlgData.FileSize
    
    DialogBoxParam(hDllInstance, MAKEINTRESOURCE(IDD_MAIN_PLUGIN), NULL, (DLGPROC)DlgProc,0);
}

// Returns the PLUGIN_NAME to PVDasm
char* PluginInfo()
{
    return PLUGIN_NAME; 
}

//////////////////////////////////////////////////////////////////////////
//                         FUNCTIONS                                    //
//////////////////////////////////////////////////////////////////////////

// Main Plugin Dialog
BOOL CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{ 	 
		case WM_INITDIALOG: 
		{
            Main_hWnd = hWnd;
            InitCommonControls();
            ShowWindow(hWnd,SW_NORMAL);
			DWORD exports=0,totalexports;
			char text[128]="",temp[128]="";
			DISASSEMBLY dasm;
			SendMessage(*PlgData.Parent_hWnd,PI_GET_NUMBER_OF_EXPORTS,(WPARAM)NULL,(LPARAM)&totalexports);
            for( DWORD i=0; i<totalexports; i++){
				SendMessage(*PlgData.Parent_hWnd,PI_GET_EXPORT_NAME,(WPARAM)i,(LPARAM)&text);
				SendMessage(*PlgData.Parent_hWnd,PI_GET_EXPORT_DASM_INDEX,(WPARAM)i,(LPARAM)&exports);
				if(exports!=-1){ // export has a valid address?
					SendMessage(*PlgData.Parent_hWnd,PI_GETASMFROMINDEX,(WPARAM)exports,(LPARAM)&dasm);
					SendMessage(*PlgData.Parent_hWnd,PI_GET_EXPORT_ORDINAL,(WPARAM)i,(LPARAM)&exports);
					wsprintf(temp,"Func: %s\nAsm: %s\nOrdinal: %02X",text,dasm.Assembly,exports);
					MessageBox(hWnd,temp,"exports",MB_OK);
				}
			}
		}
		break;

		case WM_LBUTTONDOWN: 
		{
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
        

		case WM_PAINT: // constantly painting the window
		{
			return 0;
		}
		break;
		        
		case WM_COMMAND:
		{
		   switch(LOWORD(wParam)) // what we press on?
           {
                case IDC_EXIT:
                {
                    SendMessage(hWnd,WM_CLOSE,0,0);
                }
                break;               
           }
		}
		break;
        
        case WM_CLOSE: // We colsing the Dialog
        {
          EndDialog(hWnd,0); 
        }
	    break;
	}
	return 0;
}
