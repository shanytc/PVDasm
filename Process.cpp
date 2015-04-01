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

 File: Process.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/

#include <windows.h>
#include "resource\resource.h"
#include <commctrl.h>
#include "process.h"
#include "functions.h"
#include <tlhelp32.h>
#include <vdmdbg.h>
#include <psapi.h>
#include "Resize\AnchorResizing.h"

// ================================================================
// ========================  STRUCTS  =============================
// ================================================================
typedef struct {
	DWORD			dwPID;
	PROCENUMPROC	lpProc;
	DWORD			lParam;
	BOOL			bEnd;
} EnumInfoStruct;

// ================================================================
// =====================  GLOBAL VARIABLES  =======================
// ================================================================
HWND		hWnd;
HWND		ProcessWindow;
HWND		ListModules;
HWND		hwnd;
DWORD		FileSize=0;
HBITMAP		hMenuBitmap;		// bitmap handler for menu
HINSTANCE	Original_Hinst;		// main window handler
HINSTANCE	hInstLib;			// psapi dll handler
DWORD_PTR	SelectedRow;		// selected row
int			ModuleIndex;		// print order
int			ProcessIndex;		// print order
bool		processflag;
bool		Win9x=false;		// OS Check
bool		PartialDump=false;	// partial dump active/not active
char		Text[MAX_PATH]="";

// ================================================================
// =====================  PROTOTYPES  =============================
// ================================================================

BOOL  ( WINAPI *lpfEnumProcessModules)  ( HANDLE, HMODULE*, DWORD,        LPDWORD );
DWORD ( WINAPI *lpfGetModuleFileNameEx) ( HANDLE, HMODULE , LPTSTR,       DWORD   );
DWORD ( WINAPI *lpfGetModuleInformation)( HANDLE, HMODULE , LPMODULEINFO, DWORD   );
	
// ================================================================
// ===================== Process Variable Update ==================
// ================================================================
void GetData(HWND Original, HINSTANCE hinst)
{	
	// Get original handlers of the main window from the main functino
	// this will help us to do PE Editor linking
	hwnd=Original;
	Original_Hinst=hinst;
}

// ================================================================
// ===================== Process dialog window ====================
// ================================================================
BOOL CALLBACK ProcessDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch(Message)
	{
		// This Window Message will close the dialog  //
		//============================================//
		case WM_CLOSE:
		{
			// Free bitmap handler
			DeleteObject(hMenuBitmap);
			// free loaded DLL
			FreeLibrary(hInstLib);
			// free some mem by deleting all info
			SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0);	// delete all info
			SendMessage(ListModules,LVM_DELETEALLITEMS,0,0);	// delete all info

			// reIntialize the controls on the main window
			InitializeResizeControls(hwnd);
			EndDialog(hWnd,0);
		}
		break;

		// Window Resize Intialize
		case WM_SHOWWINDOW:
		{
			InitializeResizeControls(hWnd);
		}
		break;

		// Window's Controls Resize Intialize
		case WM_SIZE:
		{
			ResizeControls(hWnd);
		}
		break;

		// Playing with the window
		case WM_LBUTTONDOWN:
		{
			ReleaseCapture();
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;

		// This Window Message is the heart of the dialog  //
		//================================================//
		case WM_INITDIALOG:
		{
            LVCOLUMN		LvCol;				// Make Coluom struct
			OSVERSIONINFO	OSVersion;			// Operating System struct
			char			namebuff[255]={0};	// Temp var
			char			stat[255]={0};		// Temp var

			Win9x=false;	// are we running on Win9x ?
			ModuleIndex=0;	// reset index of listview counter
			ProcessIndex=0; // reset index of listview counter

			// Inialize controls on which point the will be anchor too
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_LISTPROC),	GWL_USERDATA,ANCHOR_RIGHT  | ANCHOR_BOTTOM);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_MODULES),		GWL_USERDATA,ANCHOR_RIGHT  | ANCHOR_BOTTOM   | ANCHOR_NOT_TOP);
           
			// get process listview handle
			ProcessWindow=GetDlgItem(hWnd,IDC_LISTPROC);	// get the ID of the ListView
			// get module listview handle
			ListModules=GetDlgItem(hWnd,IDC_MODULES);	// get the ID of the ListView

			// Set listview properties/Styles
			SendMessage(ProcessWindow,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT|LVS_EX_FLATSB|LVS_EX_ONECLICKACTIVATE|LVS_EX_GRIDLINES); // Full row select
			SendMessage(ListModules,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT|LVS_EX_FLATSB|LVS_EX_ONECLICKACTIVATE|LVS_EX_GRIDLINES); // Full row select

			// Here we put the info on the Coulom headers
			// this is not data, only name of each header we like
			memset(&LvCol,0,sizeof(LvCol)); // set 0
			LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;	// Type of mask
			LvCol.cx=0x165;									// width between each coloum
			LvCol.pszText="Loaded Process";					// First Header
			SendMessage(ProcessWindow,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);// Insert/Show the coloum
			LvCol.cx=0x43;												// width of header
			LvCol.pszText="Process ID";									// Next coloum
			SendMessage(ProcessWindow,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol); // ...
			LvCol.cx=0x48; // width of header
			LvCol.pszText="Priority";	// Next coloum
			SendMessage(ProcessWindow,LVM_INSERTCOLUMN,2,(LPARAM)&LvCol); // ...
			// delete all items at process listview
			SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0); // delete all info

			// intialize the module's ListView
			memset(&LvCol,0,sizeof(LvCol)); // set 0
			LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
			LvCol.cx=0x100;
			LvCol.pszText="Loaded Module";
			SendMessage(ListModules,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); // Insert/Show the coloum
			LvCol.cx=0x50; // width of header
			LvCol.pszText="Address";
			SendMessage(ListModules,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
			LvCol.cx=0x57; // width of header
			LvCol.pszText="Size Of Image";
			SendMessage(ListModules,LVM_INSERTCOLUMN,2,(LPARAM)&LvCol);
			LvCol.cx=0x49; // width of header
			LvCol.pszText="Entry Point";
			SendMessage(ListModules,LVM_INSERTCOLUMN,3,(LPARAM)&LvCol);

			// Delete all items at module listview
			SendMessage(ListModules,LVM_DELETEALLITEMS,0,0);
			
			// Get Operating Sytem Information
			OSVersion.dwOSVersionInfoSize = sizeof(OSVersion);
			GetVersionEx(&OSVersion);

			// check for Win2k and above system
			if(OSVersion.dwMajorVersion==5 || OSVersion.dwMajorVersion==6 && OSVersion.dwPlatformId==VER_PLATFORM_WIN32_NT)
			{
				// Try load Psapi.Dll
				hInstLib = LoadLibraryA("PSAPI.DLL");
				// Dll Not found
				if( hInstLib == NULL )
				{
					MessageBox(hWnd,"Couldn't Load Psapi.dll","Error Loading DLL",MB_OK|MB_ICONINFORMATION);
					FreeLibrary( hInstLib );
					return false;
				}

				// load up the function from the dll
				lpfEnumProcessModules = (BOOL(WINAPI *)(HANDLE, HMODULE *,
					DWORD, LPDWORD)) GetProcAddress( hInstLib,
					"EnumProcessModules" ) ;
			    // load up the function from the dll
				lpfGetModuleFileNameEx =(DWORD (WINAPI *)(HANDLE, HMODULE,
					LPTSTR, DWORD )) GetProcAddress( hInstLib,
					"GetModuleFileNameExA" ) ;
			    // load up the function from the dll
				lpfGetModuleInformation =(DWORD (WINAPI *)(HANDLE, HMODULE,
					LPMODULEINFO, DWORD )) GetProcAddress( hInstLib,
					"GetModuleInformation" ) ;

				// Win2K is set
				Win9x=false;
			}
			else // Win9x 
				if(OSVersion.dwMajorVersion==4 && OSVersion.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS)
					Win9x=true; // Win9x is set

			// Show Process Information
			MainProcess(hWnd,ProcessWindow); 
			return true; // Always True	
		}
		break;
		
		case WM_NOTIFY:
		{
			switch(LOWORD(wParam))
			{
			    case IDC_LISTPROC:
				{
					// Create a menu while clicking right button of mouse
					if(((LPNMHDR)lParam)->code == NM_RCLICK)
					{	
						// get selected item
						SelectedRow=SendMessage(ProcessWindow,LVM_GETNEXTITEM,(WPARAM)-1,LVNI_FOCUSED); // return item selected
						
						if(SelectedRow!=-1)
						{
							DWORD PID;
							// get Process Id
							PID=GetProcessPID((DWORD)SelectedRow);
							// delete all items
							SendMessage(ListModules,LVM_DELETEALLITEMS,0,0);
							ModuleIndex=0;		// reset the order
							PrintModules(PID);	// lets get the modules and show them
						
							processflag=1;
							// load the menu and store handle
							HMENU hMenu = LoadMenu (NULL, MAKEINTRESOURCE (IDR_PROCESSES));
							// we will change onlt the sub menu
							HMENU hPopupMenu = GetSubMenu (hMenu, 0);
							POINT pt; // pointr struct
							// load bitmap to menu item
							hMenuBitmap = LoadBitmap(Original_Hinst, MAKEINTRESOURCE(IDB_PROCESS_KILL)); 
							SetMenuItemBitmaps(hPopupMenu, 0, MF_BYPOSITION, hMenuBitmap, hMenuBitmap); 
							// load bitmap to menu item
							hMenuBitmap = LoadBitmap(Original_Hinst, MAKEINTRESOURCE(IDB_PROCESS_DUMP)); 
							SetMenuItemBitmaps(hPopupMenu, 2, MF_BYPOSITION, hMenuBitmap, hMenuBitmap); 
							// load bitmap to menu item
							hMenuBitmap = LoadBitmap(Original_Hinst, MAKEINTRESOURCE(IDB_PARTIAL_DUMP)); 
							SetMenuItemBitmaps(hPopupMenu, 3, MF_BYPOSITION, hMenuBitmap, hMenuBitmap);
							// load bitmap to menu item
							hMenuBitmap = LoadBitmap(Original_Hinst, MAKEINTRESOURCE(IDB_PROCESS_PRIORITY)); 
							SetMenuItemBitmaps(hPopupMenu, 5, MF_BYPOSITION, hMenuBitmap, hMenuBitmap); 								
							// load bitmap to menu item
							hMenuBitmap = LoadBitmap(Original_Hinst, MAKEINTRESOURCE(IDB_PROCESS_REFRESH)); 
							SetMenuItemBitmaps(hPopupMenu, 6, MF_BYPOSITION, hMenuBitmap, hMenuBitmap); 								
							// cerate a default item (bold text)
							SetMenuDefaultItem (hPopupMenu, 0, TRUE);
							// get mouse position
							GetCursorPos (&pt);
							// menu will be over all window's running
							SetForegroundWindow (hWnd);
							// track the menu with the mouse pointers and show the menu/submenu
							TrackPopupMenu (hPopupMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
							// menu will be over all window's running
							SetForegroundWindow (hWnd);
							// kill menu/submenu after we showed it
							DestroyMenu (hPopupMenu);
							DestroyMenu (hMenu);
						}
					}

					if(((LPNMHDR)lParam)->code == NM_CLICK)
					{
						// left click on a process
						DWORD PID;
						// get selected item
						SelectedRow=SendMessage(ProcessWindow,LVM_GETNEXTITEM,(WPARAM)-1,(LPARAM)LVNI_FOCUSED); // return item selected
						if(SelectedRow!=-1)
						{
							// get Process ID
							PID=GetProcessPID((DWORD)SelectedRow);
							if(PID!=0)
							{
								// show all modules of the process
								SendMessage(ListModules,LVM_DELETEALLITEMS,0,0);
								ModuleIndex=0;	// reset the order
								PrintModules(PID);	// lets get the modules
							}
						}
					}
				}
				break;
			}
		}

		// This Window Message will control the dialog  //
		//==============================================//
		case WM_COMMAND:
		{
			switch(LOWORD(wParam)) // what we press on?
			{
				case ID_PROCESSES_REFRESHVIEW:
				{
					// Refresh the entire Processes SnapShot and redraw
					// set indexes
					ProcessIndex=0;
					ModuleIndex=0;
					// delete all items
					SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0); // delete all info
					SendMessage(ListModules,LVM_DELETEALLITEMS,0,0);
					// start redrawing
					MainProcess(hWnd,ProcessWindow);
				}
				break;
				
				// Set priority to REAL
				case ID_PROCESS_REAL:
				{
					DWORD  PID;
                    HANDLE hProcess;

					ProcessIndex=0;
					ModuleIndex=0;
					
					PID=GetProcessPID((DWORD)SelectedRow); // Get PID
					hProcess = OpenProcess(PROCESS_SET_INFORMATION, false, PID);
					if(hProcess!=NULL)
					{
						// Set the new Piority
						SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);
					}
					CloseHandle(hProcess);
					SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0);
					MainProcess(hWnd,ProcessWindow);
				}
				break;

				// Set priority to HIGH
				case ID_PROCESS_HIGH:
				{
					DWORD  PID;
                    HANDLE hProcess;

					ProcessIndex=0;
					ModuleIndex=0;

					PID=GetProcessPID((DWORD)SelectedRow);
					hProcess = OpenProcess(PROCESS_SET_INFORMATION, false, PID);
					if(hProcess!=NULL)
					{
						// Set the new Piority
						SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
					}
					CloseHandle(hProcess);
					SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0);
					MainProcess(hWnd,ProcessWindow);
				}
				break;

				// Set priority to NORMAL
				case ID_PROCESS_NORMAL:
				{
					DWORD  PID;
                    HANDLE hProcess;

					ProcessIndex=0;
					ModuleIndex=0;

					PID=GetProcessPID((DWORD)SelectedRow);
					hProcess = OpenProcess(PROCESS_SET_INFORMATION, false, PID);
					if(hProcess!=NULL)
					{
						// Set the new Priority
						SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
					}
					CloseHandle(hProcess);
					SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0);
					MainProcess(hWnd,ProcessWindow);
				}
				break;

				// Set priority to IDLE
				case ID_PROCESS_IDLE:
				{
					DWORD  PID;
                    HANDLE hProcess;

					ProcessIndex=0;
					ModuleIndex=0;

					PID=GetProcessPID((DWORD)SelectedRow);
					hProcess = OpenProcess(PROCESS_SET_INFORMATION, false, PID);
					if(hProcess!=NULL)
					{
						// Set the new Priority
						SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
					}
					CloseHandle(hProcess);
					SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0);
					MainProcess(hWnd,ProcessWindow);
				}
				break;

				case ID_KILL_PROCESS:
				{
					// Killing a process by its PID
					DWORD PID=0;
					HANDLE hpro=0;

					if(!processflag)
					{
						MessageBox(hWnd,"Select process to kill","Error",MB_OK);
						return 0;
					}
					
					PID=GetProcessPID((DWORD)SelectedRow); // Get PID
					if(PID!=0)
					{
						// check process access rights
					   if(!OpenProcess(PROCESS_ALL_ACCESS, 1, PID))
					   {
						  MessageBox(hWnd,"Couldn't Open Process For Killing!","Error",MB_OK);
						  return 0;
					   }
					   else
					   {
						   // open the process with all access
						  hpro=OpenProcess(PROCESS_ALL_ACCESS, 1, PID);
						  // kill the running process
						  TerminateProcess(hpro, 0);
						  // clear the process handle
						  CloseHandle(hpro);
					   }
					}
					processflag=0;

					// Reset the Process View
					SendMessage(ProcessWindow,LVM_DELETEALLITEMS,0,0); // delete all info
					SendMessage(ListModules,LVM_DELETEALLITEMS,0,0);
					Sleep(2);
					// Reset ListView Indexes
					ProcessIndex=0;
					ModuleIndex=0;
					// ReGet all Process and modules
					MainProcess(hWnd,ProcessWindow);	
				}
				break;

				case ID_DUMP_PROCESS:
				{
					// set full dump active
					PartialDump=false;
					// Full Dump, no address/size specified
					DumpFull(0,0); 
				}
				break;

				case ID_PARTIAL_DUMP:
				{
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PARTIAL_DUMP), hWnd, (DLGPROC)Partial_Dump);
				}
				break;
			}
		}
        break;
		
		default:
		{
			return FALSE;
		}
    }
	
	return TRUE;
}


//==============================================================
//=================== Partial Process Dumper====================
//==============================================================
// Dump only part of a process,
// Size and address are user defined!
BOOL CALLBACK Partial_Dump(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{ 	 
		case WM_INITDIALOG: 
		{

			LVITEM LvItem;
			HWND   WinList;

			// Mask the controls to input only user defined chars
			MaskEditControl(GetDlgItem(hWnd,IDC_BASE_ADDRESS),	"0123456789abcdef\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_DUMP_SIZE),		"0123456789abcdef\b",TRUE);
			// Limit the number of chars in the edit box
			SendDlgItemMessage(hWnd,IDC_BASE_ADDRESS,	EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_DUMP_SIZE,		EM_SETLIMITTEXT,	(WPARAM)8,0);

			// InitLvItem 
			memset(&LvItem,0,sizeof(LvItem));
			LvItem.mask=LVIF_TEXT;
			LvItem.iSubItem=0;
			LvItem.pszText=Text;
			LvItem.cchTextMax=256;
			LvItem.iItem=0;
			
			// Check operating System

			if(Win9x==false) // WinNT/2000/XP Partial Dumping
			{
				WinList=ListModules;    // Win2k++ [ read first loaded module]
				// Win2K | WinXP Dump Style
				SendMessage(WinList,LVM_GETITEMTEXT, 0, (LPARAM)&LvItem); 
				wsprintf(Text,"%s",LvItem.pszText);
				
				LvItem.iSubItem=2;
				SendMessage(WinList,LVM_GETITEMTEXT, 0, (LPARAM)&LvItem); 
				wsprintf(Text,"%s",LvItem.pszText);
				FileSize=StringToDword(Text);

				LvItem.iSubItem=1;
				SendMessage(WinList,LVM_GETITEMTEXT, 0, (LPARAM)&LvItem); 
				wsprintf(Text,"%s",LvItem.pszText);
				
				SetDlgItemText(hWnd,IDC_BASE_ADDRESS,Text);
				wsprintf(Text,"%08X",FileSize);
				SetDlgItemText(hWnd,IDC_DUMP_SIZE,Text);
				GetDlgItemText(hWnd,IDC_BASE_ADDRESS,Text,9);
			}
			else // Win9x Partial Dump
			{
				LVFINDINFO lvi;   // struct for item findinf
				DWORD_PTR      Index; // return index

				// zero the struct
				memset(&lvi,0,sizeof(lvi));
				LvItem.iSubItem=0;

				// get string to search from the process window
				SendMessage(ProcessWindow,LVM_GETITEMTEXT, SelectedRow, (LPARAM)&LvItem); 
				wsprintf(Text,"%s",LvItem.pszText); // get the name of the process
				lvi.psz=Text;
				LvItem.iSubItem=1;
				// search in loaded modules window
				Index=SendMessage(ListModules,LVM_FINDITEM, (WPARAM)-1, (LPARAM)&lvi); 	
				SendMessage(ListModules,LVM_GETITEMTEXT, Index, (LPARAM)&LvItem); 
				SetDlgItemText(hWnd,IDC_BASE_ADDRESS,LvItem.pszText);

				DWORD ProcessAddress=StringToDword(LvItem.pszText);
				LvItem.iSubItem=2;
				SendMessage(ListModules,LVM_GETITEMTEXT, Index, (LPARAM)&LvItem);
				SetDlgItemText(hWnd,IDC_DUMP_SIZE,LvItem.pszText);
				FileSize=StringToDword(LvItem.pszText);
				GetDlgItemText(hWnd,IDC_BASE_ADDRESS,Text,9); // for the partial dumping checking
			}

			return true;
		}
		break;

		case WM_CLOSE: 
		{
			// Set dump task to full
			PartialDump=false;
			EndDialog(hWnd,0); 
		}
		break;
		
		case WM_LBUTTONDOWN: 
		{
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
		
		case WM_PAINT: 
		{
			return false;
		}
		break;
		
		case WM_COMMAND: 
		{
			switch (LOWORD(wParam)) 
			{ 	 
				case ID_PARTIAL_DUMP_EXIT:
				{
					// Set dump task to full
					PartialDump=false;
					EndDialog(hWnd,0); 
				}
				break;

				case ID_PARTIAL_DUMP_SAVE:
				{
					DWORD Size,Addr,OriginalAddr;
					char Fsize[9],Address[9];

					// get new information from the edit boxes
					GetDlgItemText(hWnd,IDC_DUMP_SIZE,Fsize,9);
					GetDlgItemText(hWnd,IDC_BASE_ADDRESS,Address,9);
			
					// covnert text info to dwords
					OriginalAddr=StringToDword(Text);
					Size=StringToDword(Fsize);
					Addr=StringToDword(Address);

					// check if the dump size is out of process's limits
					if(Size>FileSize || Size==0)
					{
						MessageBox(hWnd,"Size mismatches process's boundery","Notice",MB_OK|MB_ICONINFORMATION);
						break;
					}
					else  // check if we are accessing wrong process address
						if(Addr<OriginalAddr || Addr>=(OriginalAddr+FileSize))
						{
							MessageBox(hWnd,"Address mismatches process's boundery","Notice",MB_OK|MB_ICONINFORMATION);
							break;
						}
						else
						{
							PartialDump=true;    // setting the partial dump active
							DumpFull(Size,Addr); // dump process with parameters
							PartialDump=false;   // setting partial dump inactive
							EndDialog(hWnd,0);	 // close the dailog
						}
				}
				break;
			}
			break;
		}
		break;
	}
	return 0;
}


// =============================================================
// ================= Find Running Process ======================
// ============================================================= 
BOOL WINAPI EnumProcs(PROCENUMPROC lpProc, LPARAM lParam) 
{
// The EnumProcs function takes a pointer to a callback function
// that will be called once per process with the process filename 
// and process ID.
// 
// lpProc -- Address of callback routine.
// 
// lParam -- A user-defined LPARAM value to be passed to
//           the callback routine.
// 
// Callback function definition:
// BOOL CALLBACK Proc(DWORD dw, WORD w, LPCSTR lpstr, LPARAM lParam);
	
	DWORD			dwSize;
	DWORD			dwSize2;
	DWORD			dwIndex;
	LPDWORD			lpdwPIDs  = NULL;
	OSVERSIONINFO	osver;
	HINSTANCE		hInstLib  = NULL;
	HINSTANCE		hInstLib2 = NULL;
	HANDLE			hSnapShot = NULL;
	PROCESSENTRY32	procentry;
	HMODULE			hMod;
	HANDLE			hProcess;
    EnumInfoStruct	sInfo;
	char			szFileName[MAX_PATH];
	BOOL			bFlag;
	//char           szFilePath[MAX_PATH];

	// define ToolHelp Function Pointers.
	HANDLE (WINAPI *lpfCreateToolhelp32Snapshot)(DWORD, DWORD);
	BOOL (WINAPI *lpfProcess32First)(HANDLE, LPPROCESSENTRY32);
	BOOL (WINAPI *lpfProcess32Next)(HANDLE, LPPROCESSENTRY32);

	// define PSAPI Function Pointers.
	BOOL (WINAPI *lpfEnumProcesses)(DWORD *, DWORD, DWORD *);
	BOOL (WINAPI *lpfEnumProcessModules)(HANDLE, HMODULE *, DWORD, 
		LPDWORD);
	DWORD (WINAPI *lpfGetModuleBaseName)(HANDLE, HMODULE, LPTSTR, DWORD);

	// VDMDBG Function Pointers.
	INT (WINAPI *lpfVDMEnumTaskWOWEx)(DWORD, TASKENUMPROCEX, LPARAM);

	// Retrieve the OS version
	osver.dwOSVersionInfoSize = sizeof(osver);
	if (!GetVersionEx(&osver))
		return FALSE;

	// If Windows NT == 4.0
	// If Windows  2000 == 5.0
	if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT
		&& osver.dwMajorVersion == 4) {

		// start exception handler macro 
		__try {

			// Get the procedure addresses explicitly. We do
			// this so we don't have to worry about modules
			// failing to load under OSes other than Windows NT 4.0 
			// because references to PSAPI.DLL can't be resolved.

			hInstLib = LoadLibraryA("PSAPI.DLL"); // Load Handle to PSAPI DLL
			if (hInstLib == NULL)
				__leave;
			
			hInstLib2 = LoadLibraryA("VDMDBG.DLL");// Load Handle to VDMDBG DLL
			if (hInstLib2 == NULL)
				__leave;
			
			// Get procedure addresses.
			lpfEnumProcesses = (BOOL (WINAPI *)(DWORD *, DWORD, DWORD*))
				GetProcAddress(hInstLib, "EnumProcesses");
			
			lpfEnumProcessModules = (BOOL (WINAPI *)(HANDLE, HMODULE *,
				DWORD, LPDWORD)) GetProcAddress(hInstLib,
				"EnumProcessModules");
			
			lpfGetModuleBaseName = (DWORD (WINAPI *)(HANDLE, HMODULE,
				LPTSTR, DWORD)) GetProcAddress(hInstLib,
				"GetModuleBaseNameA");
			
			lpfVDMEnumTaskWOWEx = (INT (WINAPI *)(DWORD, TASKENUMPROCEX,
				LPARAM)) GetProcAddress(hInstLib2, "VDMEnumTaskWOWEx");
			
            // check if procedures are not NULL
			if (lpfEnumProcesses == NULL 
				|| lpfEnumProcessModules == NULL 
				|| lpfGetModuleBaseName == NULL 
				|| lpfVDMEnumTaskWOWEx == NULL)
				__leave;

			//
			// Call the PSAPI function EnumProcesses to get all of the
			// ProcID's currently in the system.
			// 
			// NOTE: In the documentation, the third parameter of
			// EnumProcesses is named cbNeeded, which implies that you
			// can call the function once to find out how much space to
			// allocate for a buffer and again to fill the buffer.
			// This is not the case. The cbNeeded parameter returns
			// the number of PIDs returned, so if your buffer size is
			// zero cbNeeded returns zero.
			// 
			// NOTE: The "HeapAlloc" loop here ensures that we
			// actually allocate a buffer large enough for all the
			// PIDs in the system.
			// 
			dwSize2 = 256 * sizeof(DWORD);
			do {
				
				if (lpdwPIDs) {
					HeapFree(GetProcessHeap(), 0, lpdwPIDs);
					dwSize2 *= 2;
				}
				
                // allocate a buffer large enough for all PIDs
				lpdwPIDs = (LPDWORD) HeapAlloc(GetProcessHeap(), 0, dwSize2); 

                // check if vald
				if (lpdwPIDs == NULL)
					__leave;

                // Enumerate Processes
				if (!lpfEnumProcesses(lpdwPIDs, dwSize2, &dwSize))
					__leave;

			} while (dwSize == dwSize2);
			
			// How many ProcID's did we get?
			dwSize /= sizeof(DWORD);
			
			// Loop through each ProcID.
			for (dwIndex = 0; dwIndex < dwSize; dwIndex++) {
				szFileName[0] = 0;
				// Open the process (if we can... security does not
				// permit every process in the system to be opened).
				hProcess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					FALSE, lpdwPIDs[dwIndex]);
				if (hProcess != NULL) {
					// Here we call EnumProcessModules to get only the
					// first module in the process. This will be the 
					// EXE module for which we will retrieve the name.
					if (lpfEnumProcessModules(hProcess, &hMod,
						sizeof(hMod), &dwSize2)) {
						// Get the module name
						if (!lpfGetModuleBaseName(hProcess, hMod,
							szFileName, sizeof(szFileName)))
							szFileName[0] = 0;
					}
					CloseHandle(hProcess);
				}
				// Regardless of OpenProcess success or failure, we
				// still call the enum func with the ProcID.
				if (!lpProc(lpdwPIDs[dwIndex], 0, szFileName, lParam))
					break;

				// Did we just bump into an NTVDM?
				if (_stricmp(szFileName, "NTVDM.EXE") == 0) {

					// Fill in some info for the 16-bit enum proc.
					sInfo.dwPID = lpdwPIDs[dwIndex];
					sInfo.lpProc = lpProc;
					sInfo.lParam = (DWORD) lParam;
					sInfo.bEnd = FALSE;

					// Enum the 16-bit stuff.
					lpfVDMEnumTaskWOWEx(lpdwPIDs[dwIndex],
						(TASKENUMPROCEX) Enum16, (LPARAM) &sInfo);
					
					// Did our main enum func say quit?
					if (sInfo.bEnd)
						break;
				}
			}
			
      } __finally {
		  
		  if (hInstLib) // free dll handler
			  FreeLibrary(hInstLib);
		  
		  if (hInstLib2) // free dll handler
			  FreeLibrary(hInstLib2);
		  
		  if (lpdwPIDs) // free allocated memory (heap)
			  HeapFree(GetProcessHeap(), 0, lpdwPIDs);
      }
	  
	  // If any OS other than Windows NT 4.0. -> 2000/XP
   } else if (osver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS
	   || (osver.dwPlatformId == VER_PLATFORM_WIN32_NT
	   && osver.dwMajorVersion > 4)) {
	   
	   __try {
		   hInstLib = LoadLibraryA("Kernel32.DLL"); // handler to the kernel32.dll
		   if (hInstLib == NULL)
			   __leave;
		   // If NT-based OS, load VDMDBG.DLL.
		   /*
		   if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			   hInstLib2 = LoadLibraryA("VDMDBG.DLL");
			   if (hInstLib2 == NULL){
				   DWORD Error = GetLastError();
				   __leave;
			   }
		   }
		   */
		   // Get procedure addresses. We are linking to 
		   // these functions explicitly, because a module using
		   // this code would fail to load under Windows NT,
		   // which does not have the Toolhelp32
		   // functions in KERNEL32.DLL.
		   lpfCreateToolhelp32Snapshot =
               (HANDLE (WINAPI *)(DWORD,DWORD))
               GetProcAddress(hInstLib, "CreateToolhelp32Snapshot");

		   lpfProcess32First =
               (BOOL (WINAPI *)(HANDLE,LPPROCESSENTRY32))
               GetProcAddress(hInstLib, "Process32First");

		   lpfProcess32Next =
               (BOOL (WINAPI *)(HANDLE,LPPROCESSENTRY32))
               GetProcAddress(hInstLib, "Process32Next");

		   if (lpfProcess32Next == NULL
               || lpfProcess32First == NULL
               || lpfCreateToolhelp32Snapshot == NULL)
			   __leave;

		   /*
		   if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			   lpfVDMEnumTaskWOWEx = (INT (WINAPI *)(DWORD, TASKENUMPROCEX,
				   LPARAM)) GetProcAddress(hInstLib2, "VDMEnumTaskWOWEx");
			   if (lpfVDMEnumTaskWOWEx == NULL)
				   __leave;
		   }
		   */

		   // Get a handle to a Toolhelp snapshot of all processes.
		   hSnapShot = lpfCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		   if (hSnapShot == INVALID_HANDLE_VALUE) {
			   FreeLibrary(hInstLib);
			   return FALSE;
		   }

		   // Get the first process' information.
		   procentry.dwSize = sizeof(PROCESSENTRY32);
		   bFlag = lpfProcess32First(hSnapShot, &procentry);

		   // While there are processes, keep looping.
		   while (bFlag) {
			   // Call the enum func with the filename and ProcID.
			   if (lpProc(procentry.th32ProcessID, 0,
				   procentry.szExeFile, lParam)) {

				   // Did we just bump into an NTVDM?
				   if (_stricmp(procentry.szExeFile, "NTVDM.EXE") == 0) {

					   // Fill in some info for the 16-bit enum proc.
					   sInfo.dwPID = procentry.th32ProcessID;
					   sInfo.lpProc = lpProc;
					   sInfo.lParam = (DWORD) lParam;
					   sInfo.bEnd = FALSE;

					   // Enum the 16-bit stuff.
					   lpfVDMEnumTaskWOWEx(procentry.th32ProcessID,
						   (TASKENUMPROCEX) Enum16, (LPARAM) &sInfo);

					   // Did our main enum func say quit?
					   if (sInfo.bEnd)
						   break;
				   }

				   procentry.dwSize = sizeof(PROCESSENTRY32);
				   bFlag = lpfProcess32Next(hSnapShot, &procentry);

			   } else
				   bFlag = FALSE;
		   }
		   
	   } __finally {
		   
		   if (hInstLib)// free dll handler
			   FreeLibrary(hInstLib);

		   if (hInstLib2)// free dll handler
			   FreeLibrary(hInstLib2);
	   }

   } else
	   return FALSE;

   // Free the library.
   FreeLibrary(hInstLib);

   return TRUE;
}


BOOL WINAPI Enum16(DWORD dwThreadId, WORD hMod16, WORD hTask16,
				   PSZ pszModName, PSZ pszFileName, LPARAM lpUserDefined) {

	BOOL bRet;
	EnumInfoStruct *psInfo = (EnumInfoStruct *)lpUserDefined;
	bRet = psInfo->lpProc(psInfo->dwPID, hTask16, pszFileName,
		psInfo->lParam);

	if (!bRet) 
		psInfo->bEnd = TRUE;

	return !bRet;
} 


BOOL CALLBACK MyProcessEnumerator(DWORD dwPID, WORD wTask, LPCSTR szProcess, LPARAM lParam) 
{	
    struct processes proc;     // Processes Struct
    
	// Win9x || Win2k
	if (wTask == 0)
	{
		HANDLE hProcess;  	 // Process handler
		DWORD  hPriority;    // Priority

		// Process name/path
		wsprintf(proc.module,"%s",szProcess);
		// Process ID
		proc.pid=dwPID;	
		// Default priority is unknown
		wsprintf(proc.priority,"%s","<Unknown>");

		// Open a process for Priority reading
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, dwPID);
		// Check if process is invalid
		if(hProcess!=NULL)
		{
			// Get Process Priotiry
			hPriority=GetPriorityClass(hProcess);
			//Select priority
			switch(hPriority)
			{
			    case IDLE_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","Idle");
				break;

				case NORMAL_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","Normal");
				break;

				case HIGH_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","High");
				break;

				case REALTIME_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","Real-Time");
				break;

				default:wsprintf(proc.priority,"%s","Normal");
					break;
			}
		}
		// close the handle to the process
		CloseHandle(hProcess);
		// update the process list 
		ProcListMake(proc,hWnd,ProcessWindow);		
	}
	else // Win9x
	{
        HANDLE hProcess; // Process handler
		DWORD  hPriority;
        
		proc.pid=wTask;	 // save Process ID	 
		// Default priority is unknown
		wsprintf(proc.priority,"%s","<Unknown>");
		// save process name at struct
		wsprintf(proc.module,"%s",szProcess);
		// Get Process priority using QUERY property
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, wTask);
		// Faild to open a process
		if(hProcess!=NULL)
		{
			// Get priority
			hPriority=GetPriorityClass(hProcess);
			// Select priority
			switch(hPriority)
			{
			    case IDLE_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","Idle");
				break;

				case NORMAL_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","Normal");
				break;

				case HIGH_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","High");
				break;

				case REALTIME_PRIORITY_CLASS:
					wsprintf(proc.priority,"%s","Real-Time");
				break;

				default:wsprintf(proc.priority,"%s","Normal");
					break;
			}
		}
		// Free Process handle
		CloseHandle(hProcess);
		// Print info to the list
		ProcListMake(proc,hWnd,ProcessWindow);
	}

	// All went ok
	return true;
}

// ===================================================
// ============= Read Running Process ================
// ===================================================
void MainProcess(HWND Win,HWND ListBox)
{
	hWnd=Win;				// save Proview window handler
	ProcessWindow=ListBox;	// get the process listview handler
	// start searching for processes
	EnumProcs((PROCENUMPROC) MyProcessEnumerator, 0); 
}

// ===================================================
// === Print all Running Process onto the ListView ===
// ===================================================
static int ProcListMake(struct processes str,HWND hWnd,HWND ProcessList)
{
	//
	// Function to display the processes information
	// From the struct we declaired above.
	//
	// This function gets:
	// ===================
	//
	// 1. struct processes str: get the struct sent under new name
	// 2. HWND hWnd: Dialog Window hWnd the ListView is created on
	// 3. HWND ProcessList: Handle to the ListView that we need to refer to
	//
	
	// Insert to columns
	LVITEM LvItem;
	char   Text[255];

	memset(&LvItem,0,sizeof(LvItem));
	//===============================================
	LvItem.mask=LVIF_TEXT;
	LvItem.iItem=ProcessIndex;
	LvItem.iSubItem=0;
	wsprintf(Text,"%s",str.module);
	LvItem.pszText=Text;
	SendMessage(ProcessList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	wsprintf(Text,"%08X",str.pid);
	LvItem.iSubItem=1;
	LvItem.pszText=Text;
	SendMessage(ProcessList,LVM_SETITEM,0,(LPARAM)&LvItem);
	LvItem.iSubItem=2;
	LvItem.pszText=str.priority;
	SendMessage(ProcessList,LVM_SETITEM,0,(LPARAM)&LvItem);
	
	// Next Item to draw in
	ProcessIndex++;
	return true;
}

// ===================================================
// === Print all Running Modules onto the ListView ===
// ===================================================
void PrintListModules(char *Path, char *Address, char *Image, char *Entry)
{
	LVITEM LvItem;
	memset(&LvItem,0,sizeof(LvItem));
	//===============================================
	LvItem.mask=LVIF_TEXT;
	LvItem.iItem=ModuleIndex;
	LvItem.iSubItem=0;
	LvItem.pszText=Path;
	SendMessage(ListModules,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	LvItem.iSubItem=1;
	LvItem.pszText=Address;
	SendMessage(ListModules,LVM_SETITEM,0,(LPARAM)&LvItem);
	LvItem.iSubItem=2;
	LvItem.pszText=Image;
	SendMessage(ListModules,LVM_SETITEM,0,(LPARAM)&LvItem);
	LvItem.iSubItem=3;
	LvItem.pszText=Entry;
	SendMessage(ListModules,LVM_SETITEM,0,(LPARAM)&LvItem);
	ModuleIndex++;
}

// ===============================================
// === Get Process's Modules onto the ListView ===
// ===============================================
void PrintModules(DWORD processID)
{
	// List all the modules ( dlls ) that are
	// assosiated with the current selected process.
	// Operating System depended.

    HMODULE		hMods[1024];
    HANDLE		hProcess;
    DWORD		cbNeeded;
	MODULEINFO	mi;
    DWORD		i;
	char		Path[MAX_PATH];
	char		Addr[10];
	char		Image[10];
	char		Entry[10];
    char		szModName[MAX_PATH];

	// Open Process & Get Handle
    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
							PROCESS_VM_READ,
							FALSE, processID );
	// Failed on opening ?
    if (hProcess==NULL)
        return;

	// Win2K || WinXP system
	if(Win9x==false)
	{
		if( lpfEnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
		{
			for (i=0; i<(cbNeeded/sizeof(HMODULE)); i++) // number of modules
			{
				// Get the full path to the module's file.
				if ( lpfGetModuleFileNameEx( hProcess, hMods[i], szModName,
	                  sizeof(szModName)))
				{
					// Print the module name and handle value.
					lpfGetModuleInformation(hProcess,hMods[i],&mi,sizeof(mi));
					wsprintf(Path,"%s", szModName);
					wsprintf(Addr,"%08X",hMods[i]);
					wsprintf(Image,"%08X",mi.SizeOfImage);
					wsprintf(Entry,"%08X",mi.EntryPoint);
					PrintListModules(Path,Addr,Image,Entry);
				}
			}
		}
	}
	// Win98 || Win95 system
	else
		{
			// Snapshot all processes 
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
			// Module's struct
			MODULEENTRY32 module;
			module.dwSize = sizeof(module);

			if (Module32First(hSnapshot,&module)) // Find first module
			{
				do
				{
					// Print the module name and handle value.
					wsprintf(Path,"%s", module.szExePath);
					wsprintf(Addr,"%08X",module.modBaseAddr);
					wsprintf(Image,"%08X",module.modBaseSize);
					PrintListModules(Path,Addr,Image,"UnAvailable");
					
				} while(Module32Next(hSnapshot, &module)); // Continue to next module...
			}

			CloseHandle(hSnapshot); // Clean up handler
		}

    CloseHandle(hProcess);// Clean up handler
	
}

// =====================================================
// ========= Get Process ID From ListView Item =========
// =====================================================
DWORD GetProcessPID(int SelectedRow)
{
	// Get process ID from a selected row.
	// return value is the ID of the process ( DWORD )
	
	DWORD  PID=0;
	LVITEM LvItem;
	char   Text[10]; 

	memset(&LvItem,0,sizeof(LvItem));
	LvItem.mask=LVIF_TEXT;
	LvItem.iSubItem=1;
	LvItem.pszText=Text;
	LvItem.cchTextMax=256;
	LvItem.iItem=SelectedRow;
	
	// Get text from item 
	SendMessage(ProcessWindow,LVM_GETITEMTEXT,SelectedRow,(LPARAM)&LvItem);
	wsprintf(Text,"%s",LvItem.pszText); // copy the pid string 
	PID=StringToDword(LvItem.pszText);  // Convert string to DWORD
	
	//Return the PID of a process
	return PID; 
}

// ===============================================
// ========= Dump Process (Full/Partial) =========
// ===============================================
void DumpFull(DWORD SizeOfFile, DWORD MemoryAddress)
{
	// If PartialDump flag is set, use the SizeOfFile and MemoryAddress (OverRide defaults)
	// Else, we ignore it and dump the whole process from memory

	DWORD			MemWritten;    // Memory Written
	DWORD			ProcessAddress;
	DWORD			hFileSize=0;
	OPENFILENAME	ofn;   // File Dialog Struct handler
	HANDLE			hProcess;	// Process Handler
	HANDLE			hFile;		// File Handler (pointer)
	DWORD			PID;          // Process ID 
	LVITEM			LvItem;      // ListView Item struct
	HWND			WindowList;
	char*			MemoryData;
	char			szFileName[MAX_PATH]="";
	char			Text[255]="";
	char			Information[]="Succesfull Dump Task\nNote: you probably have to fix the dumped image!";
	
	// Intialize file saving/openning struct
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); // SEE NOTE BELOW
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Exe Files (*.exe)\0*.exe\0Dump File (*.dmp)\0*.dmp\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "exe";

	// InitLvItem 
	memset(&LvItem,0,sizeof(LvItem));
	LvItem.mask=LVIF_TEXT;
	LvItem.iSubItem=0;
	LvItem.pszText=Text;
	LvItem.cchTextMax=256;
	LvItem.iItem=0;

	// Check operating System
	if(Win9x==false)
		WindowList=ListModules; // Win2k++ [ read first loaded module] from the listview
	else
		WindowList=ProcessWindow;  // Win9x-- [ read selected process ] from the listview

		// Win2K | WinXP Dump Style
		PID=GetProcessPID((DWORD)SelectedRow); // Get PID
		if(Win9x==false)
		{
			// Get Process Address from the list view [modules]
			LvItem.iSubItem=1;
			SendMessage(WindowList,LVM_GETITEMTEXT, 0, (LPARAM)&LvItem); 
			wsprintf(Text,"%s",LvItem.pszText);
			// convert text address to DWORD address
			ProcessAddress=StringToDword(Text);
			
			LvItem.iSubItem=2;
			SendMessage(WindowList,LVM_GETITEMTEXT, 0, (LPARAM)&LvItem); 
			wsprintf(Text,"%s",LvItem.pszText);
            // Size of processes
			hFileSize=StringToDword(Text);
		}
		else if(Win9x==true) // Win95 || Win9x
		{
			LVFINDINFO lvi; // listview item find struct
			DWORD_PTR Index;
			memset(&lvi,0,sizeof(lvi));
			LvItem.iSubItem=0;

            // get text of selected row
			SendMessage(WindowList,LVM_GETITEMTEXT, (WPARAM)SelectedRow, (LPARAM)&LvItem); 
			wsprintf(Text,"%s",LvItem.pszText); // get the name of the process
			lvi.psz=Text;
			LvItem.iSubItem=1;
            // find the index of the exe's pid to dump
			Index=SendMessage(ListModules,LVM_FINDITEM,(WPARAM)-1, (LPARAM)&lvi); 	
			SendMessage(ListModules,LVM_GETITEMTEXT, Index, (LPARAM)&LvItem); 
			ProcessAddress=StringToDword(LvItem.pszText);
			LvItem.iSubItem=2;
            // get size
			SendMessage(ListModules,LVM_GETITEMTEXT, (WPARAM)Index, (LPARAM)&LvItem);
            //convert to Dword
			hFileSize=StringToDword(LvItem.pszText);
		}

		// in the case of PartialDump, we set the
		// size of dump and address, to what the user has defined at the dialogbox.
		if(PartialDump==true)
		{
			ProcessAddress=MemoryAddress;	// update new address
			hFileSize=SizeOfFile;			// update new size
		}

		// Open process as READ-ONLY for the dump process
		hProcess=OpenProcess(PROCESS_VM_READ, 1, PID);
		// invalid process?
		if(hProcess==NULL)
		{
			MessageBox(hWnd,"Faild on Opening Process for dumping!","Error Dumping!",MB_OK);
			return;
		}

		// Show file saving dialog
		if(GetSaveFileName(&ofn))
		{
			// create new file for saving
			hFile=CreateFile( szFileName,
				GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL );
			
			// File handle Invalid
			if(hFile==INVALID_HANDLE_VALUE) 
			{
				MessageBox(hWnd,"Faild on Saving..!","Error Dumping!",MB_OK);
				return;
			}
		}
		else // pressing Cancel in dialog
		{
			return;
		}
        
		// allocating enough bytes to put the memory into the array
		MemoryData=(char *)malloc(sizeof(char)*hFileSize);
		
        // is bad allocation?
		if(MemoryData==NULL)
		{
			MessageBox(hWnd,"Error Allocating Memory!","Error Dumping!",MB_OK);
			return;
		}
		
		// use Exception handling for reading from memory
		try{
			// read the process's memory address and check if it has failed
			if(ReadProcessMemory(hProcess,(LPVOID)ProcessAddress,(LPVOID)MemoryData,hFileSize,NULL)==0)
			{
				throw "Cannot Read Process Memory!"; // exception happened, throw it to the handler
			}

		}
		// catch exception
		catch(char Error[])
		{
			MessageBox(hWnd,Error,"Error Dumping!",MB_OK);
			return;
		}

		// Write/dump the read memory into the file we chosed to save to		
        if(WriteFile(hFile,MemoryData,hFileSize,&MemWritten,NULL)==0)
		{
			// if saved failed, show error
			MessageBox(hWnd,"Can't Write to File!","Error Dumping!",MB_OK);
			return;
		}

		// Clear Handles
		CloseHandle(hFile);		// close file handle
		CloseHandle(hProcess);	// close process handle
		free(MemoryData);		// release allocated memory

		// all went ok!, dump succesful
		MessageBox(hWnd,Information,"Dump Succesfully Dumped.",MB_OK|MB_ICONINFORMATION);
}